#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

/**
   @file Low level C calls to write yabe encoded data in a user managed buffer

   \b Rational

   This C library is a low level interface to write or read yabe encoded data.
   What is not implemented is storage buffer management which can vary alot
   according to the application.

   The user could have a single block buffer doubling its size when it is
   detected to be to small, he could use an array of buffers that may grow
   as needed, or he could send the buffer content through a network connection
   or apend its content into a file a and resume yabe data encoding with an
   emptied buffer.

   \b yabe writing

   All value writing are atomic except the data bytes of a string. This means
   that if there is not enough room in the buffer to write all the value bytes
   the operation fails. Since all function return the number of bytes written,
   the buffer is full if any function return a value zero in which case no
   bytes have been written in the buffer.

   The yabe_write_data() is the only exception in that it will write as much
   bytes as possible in the buffer and return the number of bytes written
   which can be less then the number of bytes requested to write. The user
   must take care to call this function again to write the remaining bytes
   until all data bytes have been written.

   If there is no enough room in the buffer to write a value and the buffer
   has a predefined fixed size, the remaining space of the buffer must be
   padded with \e none value by calling the yabe_write_none() until it returns
   zero. This ensures that reading operations will skip these bytes and
   start reading values again in the next buffer block.

   \b yabe reading

   As for writing, the yabe reading function assume yabe encoded value are
   atomic. This means that all the bytes encoding a value are contiguous in
   memory. It is an error if the buffer ends in the middle of a value. This
   error situation can be detected by the user when the yabe reading function
   returned zero and the cursor len field value is not zero.

   The only exception to this atomic value constrain are with the data bytes
   of a string. The buffer may then end anywhere in the data byte sequence,
   even before the first data byte.

   \b Encoding

   Each value is encoded as a tag byte identifying its type, followed by an
   optional value size and the value itself. When possible the size or the
   value are stored in the tag.

   \verbatim
    [tag]([size])([value])

    | Value  |    Tag    | arguments        | comment
    ---------------------------------------------------------------------
      0..127 : [0xxxxxxx]                   : integer value 0..127
      str6   : [10xxxxxx] [byte]*           : utf8 char string
      null   : [11000000]                   : null value
      int16  : [11000001] [int16]           : 16 bit integer
      int32  : [11000010] [int32]           : 32 bit integer
      int64  : [11000011] [int64]           : 64 bit integer
      flt0   : [11000100]                   : 0. float value
      flt16  : [11000101] [flt16]           : 16 bit float
      flt32  : [11000110] [flt32]           : 32 bit float
      flt64  : [11000111] [flt64]           : 64 bit float
      false  : [11001000]                   : boolean false value
      true   : [11001001]                   : boolean true value
      blob   : [11001010] [string] [string] : mime typed byte array
      ends   : [11001011]                   : equivalent to ] or }
      none   : [11001100]                   : tag byte to be ignored
      str16  : [11001101] [len16] [byte]*   : utf8 char string
      str32  : [11001110] [len32] [byte]*   : utf8 char string
      str64  : [11001111] [len64] [byte]*   : utf8 char string
      sarray : [11010xxx] [value]*          : 0 to 6 value array
      arrays : [11010111] [value]*          : equivalent to [
      sobject: [11011xxx] [str,value]*      : 0 to 6 value object
      objects: [11011111] [str,value]*      : equivalent to {
      -1..-32: [111xxxxx]                   : integer value -1..-32
    \endverbatim

* The tag is a one byte value ;
* Integer values from -32 to 127 are encoded as is as the tag value ;
* Integer values are encoded as little endian signed integer ;
* Floating point values are encoded in the IEEE 754-2008 format ;
* A strings is a sequence of utf8 encoded chars with the number of bytes as
  length ;
* A length value are encoded as little endian unsigned integer of 16, 32 or
  64 bits ;
* A blob is a pair of strings, the first is a mime type and the second is a
  sequence of raw bytes ;
* An Array is encoded as a stream of values ;
* An Object is encoded as a stream of string and value pairs where the string
  is a unique identifier ;
* An Object may not have an empty string as identifier ;
* An array or an object stream is ended by the \e ends tag ;
* If an array or an object have lest than 7 items, the \e sarray or \e sobject
  encoding should be used where the number of items is encoded in the tag and
  no \e ends tag is required ;

  \b Examples

  Writing some values.

  \code
  // Some buffer with enough space
  const size_t bufLen = 1024;
  char * buffer[bufLen];

  // A cursor where to write into the buffer
  yabe_cursor_t wCur = { buffer, bufLen };
  // msgLen keeps tracks of encoded data byte length, res if to check results
  size_t msgLen = 0, res;

  // Write yabe encoded data signature (a 5 byte constant with version)
  msgLen += res = yabe_write_signature( &wCur );
  if( !res ) { ... not done because buffer would overflow ... }

  // Write a null value (will be coded into one byte)
  msgLen += res = yabe_write_null( &wCur );
  if( !res ) { ... not done because buffer would overflow ... }

  // Write a small integer value (will be coded into one byte)
  msgLen += res = yabe_write_integer( &wCur, 123 );
  if( !res ) { ... not done because buffer would overflow ... }

  // Write a floating point value (will be coded into three byte)
  msgLen += res = yabe_write_float( &wCur, 8.5 );
  if( !res ) { ... not done because buffer would overflow ... }

  // Write a string value
  char* aString = "test string";
  size_t strLen = strlen( aString ) + 1; // include trailing '\0'
  msgLen += res = yabe_write_string( &wCur, strLen );
  if( !res ) { ... not done because buffer would overflow ... }
  msgLen += res = yabe_write_data( &wCur, aString, strLen );
  if( res != strLen ) { ... string only partially written ... }

  // msgLen is the number of bytes of encoded data which starts
  // at buffer[0];
  \endcode

  In case a writing fails, it means the data doesn't fit in the remaing free
  space of the buffer. The user could then grow the buffer, chain another
  buffer, or send or write the data to file the buffer and reset wCur to the
  start of buffer.

  If the data is encoded in a sequence of fixed size buffers referenced by an
  iovec structure for instance, the unused remaining space of buffers must be
  padded with \e none values so that these bytes will be skipped when reading
  the encoded data. The following code example shows how to do that.

  \code
  // Padding a buffer with \e none values
  while( !yabe_end_of_buffer( &wCur ) )
    yabe_write_none( &wCur );
  \endcode


  Reading the values. A yabe read operation fails if it returns 0 as the
  number of bytes read. This happens if the end of buffer is reached,
  of if the byte sequence is truncated. The later case is an error
  condition which is identified if yabe_end_of_buffer() returns false.

  \code

  // set cursor where to read data
  yabe_cursor_t rCur = { buffer, msgLen };

  // Check yabe encoded data signature (a 5 byte constant with version)
  res = yabe_read_signature( &rCur );
  if( res == 0 ) { ... invalid yabe signature or end of buffer reached ... }
  else if( res == 4 ) { ... invalid yabe encoding version ... }

  // Read a null value (will be coded into one byte)
  res = yabe_read_null( &rCur );
  if( !res ) { ... next value is not null or end of buffer reached ... }

  // Read an integer value
  int64_t intValue;
  res = yabe_write_integer( &rCur, &intValue );
  if( !res ) { ...  next value is not integer or end of buffer reached  ... }

  // Read a floating point value
  res = yabe_write_float( &rCur, 8.5 );
  if( !res ) { ... next value is not a float or end of buffer reached ... }

  // Read a string value
  size_t strLen;
  res = yabe_read_string( &rCur, &strLen );
  if( !res ) { ... next value is not a string or end of buffer reached ... }
  char* aString = malloc( strLen );
  res = yabe_read_data( &rCur, aString, strLen );
  if( res != strLen ) { ... string partially read, end of buffer reached ... }

  // Check if end of buffer reached
  if( yabe_end_of_buffer( &rCur ) ) { ... end of buffer is reached ... }
  else { ... there is some more data ... }

  \endcode

  Since different types of value may be intermixed, reading data should be
  performed by probing reading the different type of values unless the
  end of buffer was reached. If no value type mach then it is an error
  condition.

  The user could also directly trigger some action specific actions according
  to the type of value read from the yabe encoded data.

*/

/** Cursor in buffer where yabe encoded data is to be written or read */
typedef struct yabe_cursor_t
{
    char* ptr;        ///< Pointer on next byte to read or where to write
    size_t len;       ///< Number of bytes left to read or to write
} yabe_cursor_t;

/* Tag codes */
typedef char yabe_tag_t;
#define yabe_str6_tag    ((int8_t)-128)
#define yabe_null_tag    ((int8_t)-64)
#define yabe_int16_tag   ((int8_t)-63)
#define yabe_int32_tag   ((int8_t)-62)
#define yabe_int64_tag   ((int8_t)-61)
#define yabe_flt0_tag    ((int8_t)-60)
#define yabe_flt16_tag   ((int8_t)-59)
#define yabe_flt32_tag   ((int8_t)-58)
#define yabe_flt64_tag   ((int8_t)-57)
#define yabe_true_tag    ((int8_t)-56)
#define yabe_false_tag   ((int8_t)-55)
#define yabe_blob_tag    ((int8_t)-54)
#define yabe_ends_tag    ((int8_t)-53)
#define yabe_none_tag    ((int8_t)-52)
#define yabe_str16_tag   ((int8_t)-51)
#define yabe_str32_tag   ((int8_t)-50)
#define yabe_str64_tag   ((int8_t)-49)
#define yabe_sarray_tag  ((int8_t)-48)
#define yabe_arrays_tag  ((int8_t)-41)
#define yabe_sobject_tag ((int8_t)-40)
#define yabe_objects_tag ((int8_t)-33)


/** Tries writing a tag value and return the number of bytes written
 *
 * This is a low level function used internally by yabe. The user
 * must not call it.
 *
 * @param cursor[in,out] Pointer on buffer info where to write value,
 *                       update it if the value could be written
 * @param tag Tag value to write at cursor position
 * @return the number of bytes written : 0 or 1
 */
inline size_t yabe_write_tag( yabe_cursor_t* cursor, int8_t tag )
{
    if( cursor->len == 0 )
        return 0;
    *((int8_t*)cursor->ptr) = tag;
    ++cursor->ptr;
    --cursor->len;
    return 1;
}

/** Tries writing a \e null value and return the number of bytes written
 *
 * @param cursor[in,out] Pointer on buffer info where to write value,
 *                       update it if the value could be written
 * @return the number of bytes written, \e fail : 0, \e success : 1
 */
inline size_t yabe_write_null( yabe_cursor_t* cursor )
    { return yabe_write_tag( cursor, yabe_null_tag ); }


/** Tries writing a \e none tag and return the number of bytes written
 *
 * @param cursor[in,out] Pointer on buffer info where to write value,
 *                       update it if value could be written
 * @return the number of bytes written, \e fail : 0, \e success : 1
 */
inline size_t yabe_write_none( yabe_cursor_t* cursor )
    { return yabe_write_tag( cursor, yabe_none_tag ); }


/** Tries writing the boolean value and return the number of bytes written
 *
 * @param cursor[in,out] Pointer on buffer info where to write value,
 *                       update it if value could be written
 * @param value Boolean value to try writing
 * @return the number of bytes written, \e fail : 0, \e success : 1
 */
inline size_t yabe_write_bool( yabe_cursor_t* cursor, bool value )
    { return yabe_write_tag( cursor, value?yabe_true_tag:yabe_false_tag ); }


/** Tries writing the integer value and return the number of bytes written
 *
 * @param cursor[in,out] Pointer on buffer info where to write value,
 *                       update it if value could be written
 * @param value 64bit integer value to write at cursor position
 * @return the number of bytes written, \e fail : 0, \e success : 1, 3, 5 or 9
 */
size_t yabe_write_integer( yabe_cursor_t* cursor, int64_t value );


/** Tries writing the double float value and return the number of bytes written
 *
 * Denormalized floating point values are rounded to 0.
 *
 * @param cursor[in,out] Pointer on buffer info where to write value,
 *                       update it if value could be written
 * @param value 64bit floating point value to write at cursor position
 * @return the number of bytes written, \e fail : 0, \e success : 1, 3, 5 or 9
 */
size_t yabe_write_float( yabe_cursor_t* cursor, double value );


/** Tries writing the string tag and byte size values and
 *  return the number of bytes written
 *
 *  Only the tag and the string byte size are written in the buffer. The
 *  string bytes should be written using the yabe_write_data() function.
 *
 * @param cursor[in,out] Pointer on buffer info where to write value,
 *                       update it if value could be written
 * @param byteSize byte length of the utf8 encoded string
 * @return the number of bytes written, \e fail : 0, \e success : 1, 3, 5 or 9
 */
size_t yabe_write_string( yabe_cursor_t* cursor, size_t byteSize );


/** Tries writing the sequence of bytes at cursor position and
 *  return the number of bytes written.
 *
 *  The data may be partially written. A returned value smaller than \e size
 *  means the data could not be fully written.
 *
 * @param cursor[in,out] Pointer on buffer info where to write value,
 *                       update it if value could be written
 * @param data Pointer on the bytes to write at cursor
 * @param size Number of bytes to write at cursor position
 * @return the number of bytes written, , \e fail : < size, \e success : size
 */
inline size_t yabe_write_data( yabe_cursor_t* cursor, const void* data, size_t size )
{
    if( cursor->len == 0 )
        return 0;
    if( size > cursor->len )
    {
        size = cursor->len;
        memcpy( cursor->ptr, data, size );
        cursor->ptr += size;
        cursor->len = 0;
        return size;
    }
    memcpy( cursor->ptr, data, size );
    cursor->ptr += size;
    cursor->len -= size;
    return size;
}

/** Tries writing a blob tag and return the number of bytes written
 *
 * A blob must be followed by two strings. The first string encodes the mime
 * type of the blob data. The second string contains the blob data which are
 * raw bytes. The second string doesn't necessarily contain utf8 chars.
 *
 * @param cursor[in,out] Pointer on buffer info where to write value,
 *                       update it if the value could be written
 * @return the number of bytes written, \e fail : 0, \e success : 1
 */
inline size_t yabe_write_blob( yabe_cursor_t* cursor )
    { return yabe_write_tag( cursor, yabe_blob_tag ); }


/** Tries writing a small array tag and return the number of bytes written
 *
 * @param cursor[in,out] Pointer on buffer info where to write value,
 *                       update it if the value could be written
 * @param nbr Number of values in array : 0<= nbr <= 6
 * @return the number of bytes written, \e fail : 0, \e success : 1
 */
inline size_t yabe_write_small_array( yabe_cursor_t* cursor, size_t nbr )
{
    if( nbr > 6 )
        return 0;
    return yabe_write_tag( cursor, yabe_sarray_tag | nbr );
}

/** Tries writing an array stream tag and return the number of bytes written
 *
 * @param cursor[in,out] Pointer on buffer info where to write value,
 *                       update it if the value could be written
 * @return the number of bytes written, \e fail : 0, \e success : 1
 */
inline size_t yabe_write_array_stream( yabe_cursor_t* cursor )
    { return yabe_write_tag( cursor, yabe_arrays_tag ); }


/** Tries writing a small object tag and return the number of bytes written
 *
 * @param cursor[in,out] Pointer on buffer info where to write value,
 *                       update it if the value could be written
 * @param nbr Number of identfier, values pairs in object : 0<= nbr <= 6
 * @return the number of bytes written, \e fail : 0, \e success : 1
 */
inline size_t yabe_write_small_object( yabe_cursor_t* cursor, size_t nbr )
{
    if( nbr > 6 )
        return 0;
    return yabe_write_tag( cursor, yabe_sobject_tag | nbr );
}

/** Tries writing an object stream tag and return the number of bytes written
 *
 * @param cursor[in,out] Pointer on buffer info where to write value,
 *                       update it if the value could be written
 * @return the number of bytes written, \e fail : 0, \e success : 1
 */
inline size_t yabe_write_object_stream( yabe_cursor_t* cursor )
    { return yabe_write_tag( cursor, yabe_objects_tag ); }


/** Tries writing the end stream tag and return the number of bytes written
 *
 * @param cursor[in,out] Pointer on buffer info where to write value,
 *                       update it if the value could be written
 * @return the number of bytes written, \e fail : 0, \e success : 1
 */
inline size_t yabe_write_end_stream( yabe_cursor_t* cursor )
    { return yabe_write_tag( cursor, yabe_ends_tag ); }

/** Tries writing the yabe signature ['Y','A','B','E', 0]
 *
 * It requires there is at least 5 bytes available to write in the buffer.
 *
 * @param cursor[in,out] Pointer on buffer info where to write value,
 *                       update it if the value could be written
 * @return the number of bytes written, \e fail : 0, \e success : 1
 */
inline size_t yabe_write_signature( yabe_cursor_t* cursor )
    { return (cursor->len < 5) ? 0 : yabe_write_data( cursor, "YABE\0", 5 ); }

// ----------------------------------------------------------------
//
// YABE reading operations
//



/** Return true if the end of buffer is reached
 *
 * @param cursor Pointer on buffer to test and left unchanged
 * @return true if the end of buffer is reached
 */
inline bool yabe_end_of_buffer( yabe_cursor_t* cursor )
    { return cursor->len == 0; }

/** Skip \e none value in buffer if any and return the number of bytes skipped
 *
 * @param cursor[in,out] Pointer on buffer where to try reading, the cursor
 *                       is updated only if \e none tags where skipped
 * @return the number of bytes skipped
 */
inline size_t yabe_skip_none_value( yabe_cursor_t* cursor )
{
    assert( cursor && cursor->ptr );
    size_t count = 0;
    while( cursor->len > 0 && (*((int8_t*)cursor->ptr) == yabe_none_tag) )
    {
        cursor->ptr += sizeof(int8_t);
        cursor->len -= sizeof(int8_t);
        ++count;
    }
    return count;
}

/** Return the tag value without updating the cursor
 *
 * Requires the end of buffer is not reached and previous none values have
 * been skipped.
 *
 * @param cursor[in,out] Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * @return the tag value at the current cursor position
 */
inline uint8_t yabe_peek_tag( yabe_cursor_t* cursor )
{
    assert( cursor != NULL && !yabe_end_of_buffer(cursor) &&
            cursor->ptr && *((int8_t*)cursor->ptr) != yabe_none_tag);
    return *((int8_t*)cursor->ptr);
}


/** Skip tag and subsequent \e none value in buffer if any and return the
 *  total number of bytes skipped
 *
 * Requires the end of buffer is not reached and previous none values have
 * been skipped.
 * Any trailing \e none value are skipped if it succeeds.
 *
 * @param cursor[in,out] Pointer on buffer where to try skipping bytes, the
 *                       cursor is updated if bytes where skipped
 * @return the number of bytes skipped, \e fail : 0, \e success : >0
 */
inline size_t yabe_skip_tag( yabe_cursor_t* cursor )
{
    assert( cursor && !yabe_end_of_buffer(cursor) &&
            cursor->ptr && *((int8_t*)cursor->ptr) != yabe_none_tag);
    cursor->ptr += sizeof(int8_t);
    cursor->len -= sizeof(int8_t);
    return sizeof(int8_t) + yabe_skip_none_value( cursor );
}

/** Skip tag and subsequent \e none value if any if it is the same as the
 * value given as parameter and return the number of byte skipped
 *
 * Requires the end of buffer is not reached and previous none values have
 * been skipped.
 * Any trailing \e none value are skipped if it succeeds.
 *
 * @param cursor[in,out] Pointer on buffer where to try skipping bytes, the
 *                       cursor is updated if bytes where skipped
 * @return the number of bytes skipped, \e fail : 0, \e success : >0
 */
inline size_t yabe_skip_tag_if_same( yabe_cursor_t* cursor, int8_t tag )
{
    assert( cursor && !yabe_end_of_buffer(cursor) &&
            cursor->ptr && *((int8_t*)cursor->ptr) != yabe_none_tag);
    return (tag == *((int8_t*)cursor->ptr)) ? yabe_skip_tag( cursor ) : 0;
}


/** Try reading the value as null, skipping subsequent \e none values if any,
 *  and return the number of byte read
 *
 * Requires the end of buffer is not reached and previous none values have
 * been skipped.
 * Any trailing \e none value are skipped if it succeeds.
 *
 * @param cursor[in,out] Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * @return the number of bytes read, \e fail : 0, \e success : >0
 */
inline size_t yabe_read_null( yabe_cursor_t* cursor )
    { return yabe_skip_tag_if_same( cursor, yabe_null_tag ); }


/** Try reading the value as a boolean, skipping subsequent \e none values if
 *  any, and return the number of byte read
 *
 * Requires the end of buffer is not reached and previous none values have
 * been skipped.
 * Any trailing \e none value are skipped if it succeeds.
 *
 * @param cursor[in,out] Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * @param value[out] the boolean value if the value is a boolean,
 *                   otherwise value left unmodified
 * @return the number of bytes read, \e fail : 0, \e success : >0
 */
inline size_t yabe_read_bool( yabe_cursor_t* cursor, bool* value )
{
    int8_t tag = yabe_peek_tag( cursor );
    if( tag == yabe_true_tag )
    {
        *value = true;
        return yabe_skip_tag( cursor );
    }
    else if( tag == yabe_false_tag )
    {
        *value = false;
        return yabe_skip_tag( cursor );
    }
    else
        return 0;
}

/** Try reading a value as an integer, skipping subsequent \e none values if
 *  any, and return the number of byte read
 *
 * Requires the end of buffer is not reached and previous none values have
 * been skipped.
 * Any trailing \e none value are skipped if it succeeds.
 *
 * @param cursor[in,out] Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * @param value[out] the integer value if the value is an integer,
 *                   otherwise value is left unmodified
 * @return the number of bytes read, \e fail : 0, \e success : >0
 */
size_t yabe_read_integer( yabe_cursor_t* cursor, int64_t* value );

/** Try reading a value as a double float, skipping subsequent \e none values
 *  if any, and return the number of byte read
 *
 * Requires the end of buffer is not reached and previous none values have
 * been skipped.
 *
 * Assume double or float is the IEEE 754 double or float representation
 * Any trailing \e none value are skipped if it succeeds.
 *
 * @param cursor[in,out] Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * @param value[out] the double float value if the value is a double float,
 *                   otherwise value is left unmodified
 * @return the number of bytes read, \e fail : 0, \e success : >0
 */
size_t yabe_read_float( yabe_cursor_t* cursor, double* value );

/** Try reading a value as a string, skipping subsequent \e none values
 *  if any, and return the number of byte read
 *
 * Requires the end of buffer is not reached and previous none values have
 * been skipped.
 * This function simply reads the string length if it succeeds. A call to
 * yabe_read_data() is required to read the string bytes.
 * Any trailing \e none value are skipped if it succeeds.
 *
 * @param cursor[in,out] Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * @param length[out] the string length if the value is a string,
 *                   otherwise length is left unmodified
 * @return the number of bytes read, \e fail : 0, \e success : >0
 */
size_t yabe_read_string( yabe_cursor_t* cursor, size_t* length );

/** Try reading data bytes at cursor position, skipping subsequent \e none values
 *  if any, and return the number of byte read
 *
 *  Once the function has read all the data bytes it was requested to read
 *  it skips all subsequent \e none value to be ready for reading the next
 *  value.
 *  Any trailing \e none value are skipped if it completes and succeeds.
 *
 * @param cursor[in,out] Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * @param data[in,out] The pointer where to store data bytes read
 * @param size The number of bytes to read
 * @return the number of bytes read, \e fail : 0, \e success : >0
 */
inline size_t yabe_read_data( yabe_cursor_t* cursor, void* data, size_t size )
{
    if( cursor->len == 0 )
        return 0;
    if( size > cursor->len )
    {
        size = cursor->len;
        memcpy( data, cursor->ptr, size );
        cursor->ptr += size;
        cursor->len = 0;
        return size;
    }
    memcpy( data, cursor->ptr, size );
    cursor->ptr += size;
    cursor->len -= size;
    return size + yabe_skip_none_value( cursor );
}

/** Try reading the value as blob value, skipping subsequent \e none values if
 *  any, and return the number of byte read
 *
 * Requires the end of buffer is not reached and previous none values have
 * been skipped.
 *
 * A blob value is followed by two strings. The first string specifies the
 * mime type of the blob data and the second string contains the raw bytes
 * of the blob.
 * Any trailing \e none value are skipped if it succeeds.
 *
 * @param cursor[in,out] Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * @return the number of bytes read, \e fail : 0, \e success : >0
 */
inline size_t yabe_read_blob( yabe_cursor_t* cursor )
    { return yabe_skip_tag_if_same( cursor, yabe_blob_tag ); }


/** Try reading the value as a small array, skipping subsequent \e none
 *  values if any, and return the number of byte read
 *
 * Requires the end of buffer is not reached and previous none values have
 * been skipped.
 * If the returned value is none zero, this value is followed by \e number
 * values contained in the array.
 * Any trailing \e none value are skipped if it succeeds.
 *
 * @param cursor[in,out] Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * @param number[out] the number of values in the small array if the value is
 *                   a small array, otherwise value is left unmodified
 * @return the number of bytes read, \e fail : 0, \e success : >0
 */
inline size_t yabe_read_small_array( yabe_cursor_t* cursor, int8_t *number )
{
    int8_t tag = yabe_peek_tag( cursor );
    if( (tag&(uint8_t)(-8)) != yabe_sarray_tag || tag == yabe_arrays_tag )
        return 0;
    *number = tag & (uint8_t)7;
    return yabe_skip_tag( cursor );
}

/** Try reading the value as a small object, skipping subsequent \e none
 *  values if any, and return the number of byte read
 *
 * Requires the end of buffer is not reached and previous none values have
 * been skipped.
 * If the returned value is none zero, this value is followed by \e number
 * pairs of string, value contained in the object where string is a value
 * identifier.
 * Any trailing \e none value are skipped if it succeeds.
 *
 * @param cursor[in,out] Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * @param number[out] the number of values in the small object if the value is
 *                   a small array, otherwise value is left unmodified
 * @return the number of bytes read, \e fail : 0, \e success : >0
 */
inline size_t yabe_read_small_object( yabe_cursor_t* cursor, int8_t *number )
{
    int8_t tag = yabe_peek_tag( cursor );
    if( (tag&(uint8_t)(-8)) != yabe_sobject_tag || tag == yabe_objects_tag )
        return 0;
    *number = tag & (uint8_t)7;
    return yabe_skip_tag( cursor );
}

/** Try reading the value as an array stream, skipping subsequent \e none
 *  values if any, and return the number of byte read
 *
 * Requires the end of buffer is not reached and previous none values have
 * been skipped.
 * If the returned value is none zero, this value is followed by a sequence of
 * values contained in the array. The sequence ends when an end stream value
 * could be read.
 * Any trailing \e none value are skipped if it succeeds.
 *
 * @param cursor[in,out] Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * @return the number of bytes read, \e fail : 0, \e success : >0
 */
inline size_t yabe_read_array_stream( yabe_cursor_t* cursor )
    { return yabe_skip_tag_if_same( cursor, yabe_arrays_tag ); }

/** Try reading the value as an object stream, skipping subsequent \e none
 *  values if any, and return the number of byte read
 *
 * Requires the end of buffer is not reached and previous none values have
 * been skipped.
 * If the returned value is none zero, this value is followed by a sequence of
 * pairs of string,value contained in the object, where the string is the value
 * identifier. The sequence ends when an end stream value could be read.
 * Any trailing \e none value are skipped if it succeeds.
 *
 * @param cursor[in,out] Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * @return the number of bytes read, \e fail : 0, \e success : >0
 */
inline size_t yabe_read_object_stream( yabe_cursor_t* cursor )
    { return yabe_skip_tag_if_same( cursor, yabe_objects_tag ); }

/** Try reading the value as an end of stream, skipping subsequent \e none
 *  values if any, and return the number of byte read
 *
 * Requires the end of buffer is not reached and previous none values have
 * been skipped.
 * If the returned value is none zero, the end of array or object value stream
 * has been reached.
 * Any trailing \e none value are skipped if it succeeds.
 *
 * @param cursor[in,out] Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * @return the number of bytes read, \e fail : 0, \e success : >0
 */
inline size_t yabe_read_end_stream( yabe_cursor_t* cursor )
    { return yabe_skip_tag_if_same( cursor, yabe_objects_tag ); }

/** Tries reading the yabe signature ['Y','A','B','E', 0]
 *
 * It requires there are at least 5 bytes to read in the buffer;
 * Reads the first 4 bytes if they match, read also the version if it matches.
 * Any trailing \e none value are skipped if it succeeds.
 *
 * @param cursor[in,out] Pointer on buffer info where to write value,
 *                       update it if the value could be written
 * @return the number of bytes read, \e fail : 0, \e bad version : 4, \e success : >4
 */
inline size_t yabe_read_signature( yabe_cursor_t* cursor )
{
    if( cursor->len < 5 || !memcmp( cursor->ptr, "YABE", 4 ) )
        return 0;
    if( cursor->ptr[4] != 0 )
    {
        cursor->ptr += 4;
        cursor->len -= 4;
        return 4;
    }
    cursor->ptr += 5;
    cursor->len -= 5;
    return 5 + yabe_skip_none_value( cursor );
}




