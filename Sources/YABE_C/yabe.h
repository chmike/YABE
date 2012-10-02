#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

/**
   \mainpage Low level C calls to write and read YABE encoded data

   \section intro_sec Introduction

   YABE is the acronym of Yet Another Binary Encoding. The type of values it
   may encode is limited to the one of Javascript, including the \e blob type.
   It is a superset of the JSON supported data type set by the addition of
   the \e blob value type that JSON, which is text only, can't easily and
   efficiently represent.

   The rationale to choose the Javascript limited data set is because it is
   sufficient for most applications and because it makes also the encoding
   and decoding source code very small and easy to write, understand and check.
   This is also most likely the reason of the JSON encoding success.

   The benefit of a binary encoding is that marshalling is faster than with
   JSON, because \e blob values are naturally represented in it and binary
   encoding is slightly more compact than text only encoding.

   This encoding has been named YABE because there already exist a few
   encodings with similar proporties around. YABE distinguishes itself from
   them by its encoding and its API.

   \remarks This code taged YABE_v0_r0 is version 0 release 0 of the C
            source code.

   \section data_sec Data type set

   The data set is the one defined for Javascript. It is the data set
   supported by JSON which uses a text only encoding, with the addition of
   the \e blob value type that is also part of Javascript.

   A \e blob is an array of raw bytes (binary) with a mime type string.
   Since YABE is binary encoded, it can easily and efficiently encode \e blob
   values.

   \subsection atomics Atomic data values

    <ul>
    <li> \b null ;
    <li> \b Boolean : \e true & \e false ;
    <li> \b Integer : 64 bit integer ;
    <li> \b Floating \b point : 64 bit IEEE 754-2008 ;
    <li> \b String : utf8 encoded character sequence ;
    <li> \b Blob : array of raw bytes with a mime type string ;
    </ul>

   \subsection composed Composed values

    <ul>
    <li> \b Array : Sequence of any values ;
    <li> \b Object : Sequence of a pair of value identifier (string) and any
                     value ;
    </ul>

   \subsection encoding Data encoding

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

    <ul>
    <li> The tag is a one byte value ;
    <li> Integer values from -32 to 127 are encoded as is as the tag value ;
    <li> Integer values are encoded as little endian signed integer ;
    <li> Floating point values are encoded in the IEEE 754-2008 format
         (half, float, double) ;
    <li> A strings is a sequence of utf8 encoded chars with the number of bytes
         as length ;
    <li> A length value is encoded as little endian unsigned integer of 16, 32
         or 64 bits ;
    <li> A blob is a pair of strings, the first is a mime type and the second
         is a sequence of raw bytes ;
    <li> An Array is encoded as a stream of values ;
    <li> An Object is encoded as a stream of string and value pairs where the
         string is a unique identifier ;
    <li> An Object may not have an empty string as identifier ;
    <li> An array or an object stream is ended by the \e ends tag ;
    <li> If an array or an object have less than 7 items, the \e sarray or
         \e sobject encoding should be used where the number of items is
         encoded in the tag and there is no \e ends tag ;
    </ul>

   \subsection signature YABE encoded block signature

   A 5 byte signature may start a byte block containing YABE encoded data. The
   first four bytes are the ASCII code 'Y', 'A', 'B', 'E' in that order, and
   the fifth byte is the version number of the encoding. This short
   specification describes the encoding version 0.

   \remarks The size of a YABE encoded data block must be determined by the
            context.

   \section api YABE writing and reading API

   The yabe.h and yabe.c files provide low level C functions to write and
   read YABE encoded data. The provided code doesn't manage the buffer storage
   because there are too many different ways to do this.

   The user may want to grow the buffer as needed, append a new buffer block
   to a chain of block, send through the network or write to file the filled
   buffer and resume with the buffer emptied, etc.

   In the C API functions, YABE writing and reading use a \e cursor to keep
   track where to write or read data in memory. When writing, the cursor holds
   a pointer on position in memory where to write and the number of free
   writable bytes in the buffer. When reading, the cursor holds a pointer on
   the position in memory where the next data to read is located and the number
   of bytes left to read in the buffer. The user is responsible to initialize
   the cursor accordingly.

   All reading and writing functions return the number of bytes read or
   written. The operation has thus failed if the returned value is 0. When
   reading, the end of buffer should have been reached. If not, then an error
   occured in the decoding.

   \subsection writing YABE data writing

   All YABE encoded values are written in contiguous bytes in the buffer. This
   is called atomic values writing. If there is not enough room in the buffer
   to write the value bytes, the operation failes and return 0 as the number of
   bytes written. Atomic values can be at most 9 bytes long.

   The only exception is the yabe_write_data() functions which writes as much
   data bytes as possible and will thus never fail. If all the bytes could not
   be written, the user must resume the operation by a new call to write the
   remaining data bytes when new buffer space has been made available.

   If YABE encoded data is to be written in an array of buffers with predefined
   fixed size, the remaining space of the buffer must be padded with \e none
   value which needs only a single byte as storage space. \e None values are
   silently skipped when reading YABE encoded data.

   \subsection reading YABE data reading

   Since any type of value can be stored at any position in a YABE encoded
   stream, this API should be used in the following way
   <ol>
   <li> while there is data left to read from the YABE encoded stream ;
   <li> for each possible type of data with \e none value as a last resort ;
   <li> try reading the value
   <li> if the return value is not 0 (it succeeded), resume with step 1 to
        read the next value ;
   <li> otherwise a fatal error occured for one of the following reasons which
        can't be distinguished:
       <ul>
       <li> the value to read has been trucated ;
       <li> a previous decoding error made reading out of sync ;
       <li> data is not YABE encoded data.
       </ul>
   </ol>

   Some values implies to be followed by a number of other values, sometimes
   with a well defined type. It is the case for blobs that must be followed
   by two strings and for arrays and objects as well. It is the user
   responsibility to very that this implicit rules are respected.

   \section examples Examples

   \subsection writing_example Writing some values.

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
    // Padding a buffer with none values
    while( !yabe_end_of_buffer( &wCur ) )
        yabe_write_none( &wCur );
   \endcode

   \subsection reading_example Reading some values

   The following example illustrates how to read different types of values.
   However when reading YABE encoded data, the user should implement
   \ref reading ''this algorithm''.


   \code
    // set cursor where to read data
    yabe_cursor_t rCur = { data, size };
    size_t res;

    // Check yabe encoded data signature (a 5 byte constant with version)
    res = yabe_read_signature( &rCur );
    if( res == 0 ) { ... invalid yabe signature or end of buffer reached ... }
    else if( res == 4 ) { ... invalid yabe encoding version ... }
    assert( res == 5 );

    // Read a null value (will be coded into one byte)
    res = yabe_read_null( &rCur );
    if( !res ) { ... next value is not null or end of buffer reached ... }
    assert( res == 1 );

    // Read an integer value
    int64_t intValue;
    res = yabe_write_integer( &rCur, &intValue );
    if( !res ) { ...  next value is not integer or end of buffer reached  ... }
    assert( res == 1 || res == 3 || res == 5 || res == 9 );

    // Read a floating point value
    res = yabe_write_float( &rCur, 8.5 );
    if( !res ) { ... next value is not a float or end of buffer reached ... }
    assert( res == 1 || res == 3 || res == 5 || res == 9 );

    // Read a string
    size_t strLen;
    res = yabe_read_string( &rCur, &strLen ); // read the string length
    if( !res ) { ... next value is not a string or end of buffer reached ... }
    assert( res == 1 || res == 3 || res == 5 || res == 9 );
    char* aString = malloc( strLen ); // get a storage for the string
    res = yabe_read_data( &rCur, aString, strLen );
    if( res != strLen ) { ... string partially read, end of buffer reached ... }

    // Check if end of buffer reached
    if( yabe_end_of_buffer( &rCur ) ) { ... end of buffer is reached ... }
    else { ... there is some more data ... }

   \endcode
*/

/**
 * \brief Cursor in buffer where yabe encoded data is to be written or read */
typedef struct yabe_cursor_t
{
    char* ptr;        ///< Pointer on next byte to read or where to write
    size_t len;       ///< Number of bytes left to read or to write
} yabe_cursor_t;

/// @cond DEV
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

// ----------------------------------------------------------------
//
//                YABE reading functions
//
// ----------------------------------------------------------------

/**
 * \brief Tries writing a tag value and returns the number of bytes written
 *
 * This is a low level function used internally by yabe. The user
 * must not call it.
 *
 * \param[in,out] cursor Pointer on buffer info where to write value,
 *                       update it if the value could be written
 * \param tag Tag value to write at cursor position
 * \return the number of bytes written : 0 or 1
 */
static inline size_t yabe_write_tag( yabe_cursor_t* cursor, int8_t tag )
{
    if( cursor->len == 0 )
        return 0;
    *((int8_t*)cursor->ptr) = tag;
    ++cursor->ptr;
    --cursor->len;
    return 1;
}
/// @endcond


/**
 * \brief Tries writing a \e none value and returns the number of bytes written
 *
 * The purpose of this value is to be used as padding or to overwrite
 * encoded values so that they are ignored when reading.
 *
 * \param[in,out] cursor Pointer on buffer info where to write value,
 *                       update it if value could be written
 * \return the number of bytes written, \e fail : 0, \e success : 1
 */
static inline size_t yabe_write_none( yabe_cursor_t* cursor )
    { return yabe_write_tag( cursor, yabe_none_tag ); }


/**
 * \brief Tries writing a \e null value and returns the number of bytes written
 *
 * \param[in,out] cursor Pointer on buffer info where to write value,
 *                       update it if the value could be written
 * \return the number of bytes written, \e fail : 0, \e success : 1
 */
static inline size_t yabe_write_null( yabe_cursor_t* cursor )
    { return yabe_write_tag( cursor, yabe_null_tag ); }


/**
 * \brief Tries writing the boolean value and returns the number of bytes written
 *
 * \param[in,out] cursor Pointer on buffer info where to write value,
 *                       update it if value could be written
 * \param value Boolean value to try writing at cursor position
 * \return the number of bytes written, \e fail : 0, \e success : 1
 */
static inline size_t yabe_write_bool( yabe_cursor_t* cursor, bool value )
    { return yabe_write_tag( cursor, value?yabe_true_tag:yabe_false_tag ); }


/**
 * \brief Tries writing the integer value and returns the number of bytes written
 *
 * \param[in,out] cursor Pointer on buffer info where to write value,
 *                       update it if value could be written
 * \param value 64bit integer value to try writing at cursor position
 * \return the number of bytes written, \e fail : 0, \e success : 1, 3, 5 or 9
 */
size_t yabe_write_integer( yabe_cursor_t* cursor, int64_t value );


/**
 * \brief Tries writing the double float value and returns the number of bytes
 *  written
 *
 * Denormalized floating point values are rounded to 0.
 *
 * \param[in,out] cursor Pointer on buffer info where to write value,
 *                       update it if value could be written
 * \param value 64bit floating point value to try writing at cursor position
 * \return the number of bytes written, \e fail : 0, \e success : 1, 3, 5 or 9
 */
size_t yabe_write_float( yabe_cursor_t* cursor, double value );


/**
 * \brief Tries writing the string tag and its byte size values and returns the
 *  number of bytes written
 *
 *  Only the string value tag and the string byte size are written at the
 *  cursor position. The string bytes (utf8 chars) must be written using the
 *  yabe_write_data() function.
 *
 * \param[in,out] cursor Pointer on buffer info where to write value,
 *                       update it if value could be written
 * \param byteSize byte length of the utf8 encoded string to try writing at
 *                 cursor position
 * \return the number of bytes written, \e fail : 0, \e success : 1, 3, 5 or 9
 */
size_t yabe_write_string( yabe_cursor_t* cursor, size_t byteSize );


/**
 * \brief Writes as much bytes as possible at cursor position until all bytes of the
 *  sequence are written or the end of buffer is met, and returns the number
 *  of bytes written.
 *
 *  The data may be partially written. A returned value smaller than \e size
 *  means the data could not be fully written. One or more additionnal calls
 *  to this function are required to write the remaining bytes of data.
 *
 * \param[in,out] cursor Pointer on buffer info where to write value,
 *                       update it if value could be written
 * \param data Pointer on the bytes to writing at
 *                 cursor position
 * \param size Number of bytes to write at cursor position
 * \return the number of bytes written, \e incomplete : < \e size,
 *         \e complete : \e size
 */
static inline size_t yabe_write_data( yabe_cursor_t* cursor, const void* data, size_t size )
{
    if( cursor->len == 0 || size == 0 )
        return 0;
    if( size > cursor->len )
        size = cursor->len;
    memcpy( cursor->ptr, data, size );
    cursor->ptr += size;
    cursor->len -= size;
    return size;
}


/**
 * \brief Tries writing a blob tag and returns the number of bytes written
 *
 * A blob \e must be followed by two strings. The first string encodes the mime
 * type of the blob data. The second string contains the blob data made of
 * raw bytes. The second string doesn't necessarily contain utf8 chars.
 *
 * \param[in,out] cursor Pointer on buffer info where to write value,
 *                       update it if the value could be written
 * \return the number of bytes written, \e fail : 0, \e success : 1
 */
static inline size_t yabe_write_blob( yabe_cursor_t* cursor )
    { return yabe_write_tag( cursor, yabe_blob_tag ); }


/**
 * \brief Tries writing a small array tag and returns the number of bytes written
 *
 * \param[in,out] cursor Pointer on buffer info where to write value,
 *                       update it if the value could be written
 * \param nbr Number of values in array : 0<= nbr <= 6
 * \return the number of bytes written, \e fail : 0, \e success : 1
 */
static inline size_t yabe_write_small_array( yabe_cursor_t* cursor, size_t nbr )
    { return (nbr > 6) ? 0 : yabe_write_tag( cursor, yabe_sarray_tag|nbr ); }


/**
 * \brief Tries writing an array stream tag and returns the number of bytes written
 *
 * \param[in,out] cursor Pointer on buffer info where to write value,
 *                       update it if the value could be written
 * \return the number of bytes written, \e fail : 0, \e success : 1
 */
static inline size_t yabe_write_array_stream( yabe_cursor_t* cursor )
    { return yabe_write_tag( cursor, yabe_arrays_tag ); }


/**
 * \brief Tries writing a small object tag and returns the number of bytes written
 *
 *\param[in,out] cursor Pointer on buffer info where to write value,
 *                       update it if the value could be written
 * \param nbr Number of identfier, values pairs in object : 0<= nbr <= 6
 * \return the number of bytes written, \e fail : 0, \e success : 1
 */
static inline size_t yabe_write_small_object( yabe_cursor_t* cursor, size_t nbr )
    { return (nbr > 6) ? 0 : yabe_write_tag( cursor, yabe_sobject_tag|nbr ); }


/**
 * \brief Tries writing an object stream tag and returns the number of bytes written
 *
 * \param[in,out] cursor Pointer on buffer info where to write value,
 *                       update it if the value could be written
 * \return the number of bytes written, \e fail : 0, \e success : 1
 */
static inline size_t yabe_write_object_stream( yabe_cursor_t* cursor )
    { return yabe_write_tag( cursor, yabe_objects_tag ); }


/**
 * \brief Tries writing the end stream tag and returns the number of bytes written
 *
 * \param[in,out] cursor Pointer on buffer info where to write value,
 *                       update it if the value could be written
 * \return the number of bytes written, \e fail : 0, \e success : 1
 */
static inline size_t yabe_write_end_stream( yabe_cursor_t* cursor )
    { return yabe_write_tag( cursor, yabe_ends_tag ); }


/**
 * \brief Tries writing the yabe signature ['Y','A','B','E', 0] and returns
 *  the number of bytes written
 *
 * \param[in,out] cursor Pointer on buffer info where to write value,
 *                       update it if the value could be written
 * \return the number of bytes written, \e fail : 0, \e success : 5
 */
static inline size_t yabe_write_signature( yabe_cursor_t* cursor )
    { return (cursor->len < 5) ? 0 : yabe_write_data( cursor, "YABE\0", 5 ); }



// ----------------------------------------------------------------
//
//                YABE reading functions
//
// ----------------------------------------------------------------


/**
 * \brief Return true if the end of buffer is reached, false otherwise
 *
 * \param cursor Pointer on buffer to test
 * \return true if the end of buffer is reached, false otherwise
 */
static inline bool yabe_end_of_buffer( const yabe_cursor_t* cursor )
    { assert( cursor ); return cursor->len == 0; }


/// @cond DEV
/**
 * \brief Return the tag value at cursor position without updating the cursor
 *
 * Requires the end of buffer is not reached and the value at current
 * cursor position is not a \e none value.
 *
 * \param[in,out] cursor Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * \return the tag value at the current cursor position
 */
static inline uint8_t yabe_peek_tag( const yabe_cursor_t* cursor )
{
    assert( cursor && cursor->ptr && cursor->len );
    return *((int8_t*)cursor->ptr);
}


/**
 * \brief Skip a tag byte a cursor position and return 1 as the number of bytes read
 *
 * Requires the cursor is not at the end of buffer when the function is called.
 *
 * \param[in,out] cursor Pointer on the tag to skip in the buffer, the cursor
 *                       is updated to point on the next byte in the buffer
 * \return 1 as the number of bytes "read"
 */
static inline size_t yabe_skip_tag( yabe_cursor_t* cursor )
{
    assert( cursor && cursor->ptr && cursor->len );
    cursor->ptr += sizeof(int8_t);
    cursor->len -= sizeof(int8_t);
    return sizeof(int8_t);
}


/**
 * \brief Skip tag if it is the same as the argument and returns the number of byte
 *  skipped
 *
 * Requires the cursor is not at the end of buffer when the function is called.
 *
 * \param[in,out] cursor Pointer on buffer where to try skipping bytes, the
 *                       cursor is updated if bytes where skipped
 * \return the number of bytes skipped, \e fail : 0, \e success : 1
 */
static inline size_t yabe_skip_tag_if_is( yabe_cursor_t* cursor, int8_t tag )
    { return (yabe_peek_tag(cursor) == tag) ? yabe_skip_tag( cursor ) : 0; }
/// @endcond


/**
 * \brief Skip \e none value in buffer if any and returns the number of bytes skipped
 *
 * \param[in,out] cursor Pointer on buffer where to try reading, the cursor
 *                       is updated only if \e none tags where skipped
 * \return the number of bytes skipped
 */
static inline size_t yabe_read_none( yabe_cursor_t* cursor )
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


/**
 * \brief Try reading the value as null and returns the number of byte read
 *
 * Requires the cursor is not at the end of buffer when the function is called.
 *
 * \param[in,out] cursor Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * \return the number of bytes read, \e fail : 0, \e success : 1
 */
static inline size_t yabe_read_null( yabe_cursor_t* cursor )
    { return yabe_skip_tag_if_is( cursor, yabe_null_tag ); }


/**
 * \brief Try reading the value as a boolean and returns the number of byte read
 *
 * Requires the cursor is not at the end of buffer when the function is called.
 *
 * \param[in,out] cursor Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * \param[out] value the boolean value if the value is a boolean,
 *                   otherwise the value is left unchanged
 * \return the number of bytes read, \e fail : 0, \e success : 1
 */
static inline size_t yabe_read_bool( yabe_cursor_t* cursor, bool* value )
{
    int8_t tag = yabe_peek_tag( cursor );
    if( tag == yabe_true_tag )
        *value = true;
    else if( tag == yabe_false_tag )
        *value = false;
    else
        return 0;
    return yabe_skip_tag( cursor );
}


/**
 * \brief Try reading the value as an integer, skipping subsequent \e none values if
 *  any, and returns the number of byte read
 *
 * Requires the cursor is not at the end of buffer when the function is called.
 *
 * \param[in,out] cursor Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * \param[out] value the integer value if the value is an integer,
 *                   otherwise the value is left unchanged
 * \return the number of bytes read, \e fail : 0, \e success : 1, 3, 5, 9
 */
size_t yabe_read_integer( yabe_cursor_t* cursor, int64_t* value );


/**
 * \brief Try reading the value as a double float and returns the number of byte read
 *
 * Requires the cursor is not at the end of buffer when the function is called.
 * Assume double or float is the IEEE 754 double or float representation
 *
 * \param[in,out] cursor Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * \param[out] value the double float value if the value is a double float,
 *                   otherwise the value is left unchanged
 * \return the number of bytes read, \e fail : 0, \e success : 1, 3, 5, 9
 */
size_t yabe_read_float( yabe_cursor_t* cursor, double* value );


/**
 * \brief Try reading the value as a string and returns the number of byte read
 *
 * Requires the cursor is not at the end of buffer when the function is called.
 *
 * \remarks This function only reads the string byte length if it succeeds. A
 * subsequent call to yabe_read_data() is required to read the string bytes.
 *
 * \param[in,out] cursor Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * \param[out] length the string byte length if the value is a string,
 *                   otherwise the length value is left unchanged
 * \return the number of bytes read, \e fail : 0, \e success : 1, 3, 5, 9
 */
size_t yabe_read_string( yabe_cursor_t* cursor, size_t* length );


/**
 * \brief Try reading the requested number of data bytes at the cursor position
 *  and returns the number of bytes effectively read
 *
 *  Once the function has read all the data bytes it was requested to read
 *  it skips all subsequent \e none value to be ready for reading the next
 *  value.
 *
 * \param[in,out] cursor Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * \param[out] data The pointer where to store data bytes read
 * \param size The number of bytes to read
 * \return the number of bytes read, \e incomplete : < size, \e complete : size
 */
static inline size_t yabe_read_data( yabe_cursor_t* cursor, void* data, size_t size )
{
    if( cursor->len == 0 || size == 0 )
        return 0;
    if( size > cursor->len )
        size = cursor->len;
    memcpy( data, cursor->ptr, size );
    cursor->ptr += size;
    cursor->len -= size;
    return size;
}


/**
 * \brief Try reading the value as blob and returns the number of byte read
 *
 * Requires the cursor is not at the end of buffer when the function is called.
 *
 * A blob value is followed by two strings. The first string specifies the
 * mime type of the blob data and the second string contains the raw bytes
 * of the blob.
 *
 * \param[in,out] cursor Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * \return the number of bytes read, \e fail : 0, \e success : 1
 */
static inline size_t yabe_read_blob( yabe_cursor_t* cursor )
    { return yabe_skip_tag_if_is( cursor, yabe_blob_tag ); }


/**
 * \brief Try reading the value as a small array and returns the number of byte read
 *
 * Requires the cursor is not at the end of buffer when the function is called.
 *
 * If the returned value is none zero, it is followed by \e number
 * YABE encoded values corresponding to the items of the array.
 *
 * \param[in,out] cursor Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * \param[out] number the number of items in the small array if the read
 *                    succeeds, otherwise number is left unchanged
 * \return the number of bytes read, \e fail : 0, \e success : 1
 */
static inline size_t yabe_read_small_array( yabe_cursor_t* cursor, int8_t *number )
{
    int8_t tag = yabe_peek_tag( cursor );
    if( (tag&~(uint8_t)7) != yabe_sarray_tag || tag == yabe_arrays_tag )
        return 0;
    *number = tag & (uint8_t)7;
    return yabe_skip_tag( cursor );
}


/**
 * \brief Try reading the value as a small object and returns the number of byte read
 *
 * Requires the cursor is not at the end of buffer when the function is called.
 *
 * If the returned value is none zero, it is followed by \e number
 * of string and YABE encoded values pairs corresponding to the items of the
 * object.
 *
 * \param[in,out] cursor Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * \param[out] number the number of items in the small object if the read
 *                    succeeds, otherwise number is left unchanged
 * \return the number of bytes read, \e fail : 0, \e success : 1
 */
static inline size_t yabe_read_small_object( yabe_cursor_t* cursor, int8_t *number )
{
    int8_t tag = yabe_peek_tag( cursor );
    if( (tag&(uint8_t)(-8)) != yabe_sobject_tag || tag == yabe_objects_tag )
        return 0;
    *number = tag & (uint8_t)7;
    return yabe_skip_tag( cursor );
}


/**
 * \brief Try reading the value as an array stream, skipping subsequent \e none
 *  values if any, and returns the number of byte read
 *
 * Requires the cursor is not at the end of buffer when the function is called.
 * If the returned value is none zero, this value is followed by a sequence of
 * values contained in the array. The sequence ends when an end stream value
 * could be read.
 *
 * \param[in,out] cursor Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * \return the number of bytes read, \e fail : 0, \e success : 1
 */
static inline size_t yabe_read_array_stream( yabe_cursor_t* cursor )
    { return yabe_skip_tag_if_is( cursor, yabe_arrays_tag ); }


/**
 * \brief Try reading the value as an object stream, skipping subsequent \e none
 *  values if any, and returns the number of byte read
 *
 * Requires the cursor is not at the end of buffer when the function is called.
 * If the returned value is none zero, this value is followed by a sequence of
 * pairs of string,value contained in the object, where the string is the value
 * identifier. The sequence ends when an end stream value could be read.
 *
 * \param[in,out] cursor Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * \return the number of bytes read, \e fail : 0, \e success : 1
 */
static inline size_t yabe_read_object_stream( yabe_cursor_t* cursor )
    { return yabe_skip_tag_if_is( cursor, yabe_objects_tag ); }


/**
 * \brief Try reading the value as an end of stream, skipping subsequent \e none
 *  values if any, and returns the number of byte read
 *
 * Requires the cursor is not at the end of buffer when the function is called.
 * If the returned value is none zero, the end of array or object value stream
 * has been reached.
 *
 * \param[in,out] cursor Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * \return the number of bytes read, \e fail : 0, \e success : 1
 */
static inline size_t yabe_read_end_stream( yabe_cursor_t* cursor )
    { return yabe_skip_tag_if_is( cursor, yabe_objects_tag ); }


/**
 * \brief Try reading the yabe signature ['Y','A','B','E', 0]
 *
 * It requires there are at least 5 bytes to read in the buffer.
 * Reads the first 4 bytes if they match, read also the version if it matches.
 *
 * \param[in,out] cursor Pointer on buffer where to try reading, the cursor
 *                       is updated if the read operation succeeds
 * \return the number of bytes read, \e fail : 0, \e bad version : 4, \e success : 5
 */
static inline size_t yabe_read_signature( yabe_cursor_t* cursor )
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
    return 5;
}




