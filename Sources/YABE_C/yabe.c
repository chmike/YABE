#include "yabe.h"


/* Low level buffer writing operation. Note : cursor->len left unchanged */
inline void yabe_poke_int8( yabe_cursor_t* cursor, int8_t val )
    {  *((int8_t*)cursor->ptr) = val; cursor->ptr += sizeof(int8_t); }

inline void yabe_poke_int16( yabe_cursor_t* cursor, int16_t val )
    { *((int16_t*)cursor->ptr) = val; cursor->ptr += sizeof(int16_t); }

inline void yabe_poke_int32( yabe_cursor_t* cursor, int32_t val )
    { *((int32_t*)cursor->ptr) = val; cursor->ptr += sizeof(int32_t); }

inline void yabe_poke_int64( yabe_cursor_t* cursor, int64_t val )
    { *((int64_t*)cursor->ptr) = val; cursor->ptr += sizeof(int64_t); }

inline void yabe_poke_uint16( yabe_cursor_t* cursor, uint16_t val )
    { *((uint16_t*)cursor->ptr) = val; cursor->ptr += sizeof(int16_t); }

inline void yabe_poke_uint32( yabe_cursor_t* cursor, uint32_t val )
    { *((uint32_t*)cursor->ptr) = val; cursor->ptr += sizeof(uint32_t); }

inline void yabe_poke_uint64( yabe_cursor_t* cursor, uint64_t val )
    { *((uint64_t*)cursor->ptr) = val;  cursor->ptr += sizeof(uint64_t); }


/* Return true if could write integer into buffer,
   otherwise return the number bytes written */
size_t yabe_write_integer( yabe_cursor_t* cursor, int64_t val )
{
    if( val >= -32 && val <= 127 )
    {
        if( cursor->len == 0 )
            return 1;
        yabe_poke_int8( cursor, (int8_t)val);
        cursor->len -= sizeof(int8_t);
        return sizeof(int8_t);
    }
    if( val >= -32768 && val <= 32767 )
    {
        const size_t len = sizeof(int8_t) + sizeof(int16_t);
        if( cursor->len < len )
            return 0;
        yabe_poke_int8( cursor, yabe_int16_tag);
        yabe_poke_int16( cursor, (int16_t)val);
        cursor->len -= len;
        return len;
    }
    if( val >= -2147483648LL && val <= 2147483647LL )
    {
        const size_t len = sizeof(int8_t) + sizeof(int32_t);
        if( cursor->len < len )
            return 0;
        yabe_poke_int8( cursor, yabe_int32_tag);
        yabe_poke_int32( cursor, (int32_t)val);
        cursor->len -= len;
        return len;
    }
    const size_t len = sizeof(int8_t) + sizeof(int64_t);
    if( cursor->len < len )
        return 0;
    yabe_poke_int8( cursor, yabe_int64_tag);
    yabe_poke_int64( cursor, val);
    cursor->len -= len;
    return len;
}

/* Return 0 if could write a floating point value at cursor position,
   otherwise return the number bytes written */
size_t yabe_write_float( yabe_cursor_t* cursor, double val )
{
    // 16bit float e=5bits m=10bits e=(-14..15)+15
    // 32bit float e=8bits m=23bits e=(-126..127)+127
    // 64bit float e=11bits m=52bits e=(-1026..1027)+1023
    // Check sign mismatch and if float maps to double

    const int64_t EXPONENT_BITS = 0x7FFULL<<52;

    // get float value as int64 value
    double* vPtr = &val;
    int64_t dr = *((int64_t*)vPtr);

    // if value is 0., write flt0 tag
    if( (dr & 0x7FFFFFFFFFFFFFFFULL) == 0 )
    {
        const size_t len = sizeof(int8_t);
        if( cursor->len < len )
            return 0;
        yabe_poke_int8( cursor, yabe_flt0_tag);
        cursor->len -= len;
        return len;
    }

    // get exponent bits from int64 value and clear sign bit
    int64_t de = dr & EXPONENT_BITS;

    // if value is infinity or NaN (exponent has all bit set), write as flt16
    if( de == EXPONENT_BITS )
    {
        const size_t len = sizeof(int8_t) + sizeof(int16_t);
        if( cursor->len < len )
            return 0;

        // if mantissa is not 0, write NaN, else write signed infinity
        uint16_t hr;
        if( dr & 0xFFFFFFFFFFFFFLL )
            hr = 0x7D00; // normalized NaN
        else if( dr < 0 )
            hr = 0xFC00; // - infinity
        else
            hr = 0x7C00; // + infinity

        yabe_poke_int8( cursor, yabe_flt16_tag);
        yabe_poke_uint16( cursor, hr );
        cursor->len -= len;
        return len;
    }

    // Get exponent value
    int16_t he = (de >> 52) - 1023;

    // if value fits in flt16, write it as flt16
    if( he >= -14 && he <= 15 && (dr & 0x3FFFFFFFFFFLL) == 0 )
    {
        const size_t len = sizeof(int8_t) + sizeof(int16_t);
        if( cursor->len < len )
            return 0;

        // initialize output value v with exponent bits
        uint16_t hr = (uint16_t)(he + 15) << 10;

        // set value sign bit
        if( dr < 0 )
            hr |= 0x8000;

        // set mantissa bits
        hr |= ((uint16_t)(dr>>(52-10)))&0x3FF;

        // write 16bit float value
        yabe_poke_int8( cursor, yabe_flt16_tag);
        yabe_poke_uint16( cursor, hr );
        cursor->len -= len;
        return len;
    }

    // if value fit in flt32, write it as flt32
    if( he >=-126 && he <= 127 && (dr & 0x1FFFFFFFLL) == 0 )
    {
        const size_t len = sizeof(int8_t) + sizeof(int32_t);
        if( cursor->len < len )
            return 0;

        // initialize output value v with exponent bits
        uint32_t fr = (uint32_t)(he + 127) << 23;

        // set value sign bit
        if( dr < 0 )
            fr |= 0x80000000;

        // set mantissa bits
        fr |= ((uint32_t)(dr>>29))&0x7FFFFF;

        // write 32bit float value
        yabe_poke_int8( cursor, yabe_flt32_tag);
        yabe_poke_uint32( cursor, fr );
        cursor->len -= len;
        return len;
    }

    const size_t len = sizeof(int8_t) + sizeof(int64_t);
    if( cursor->len < len )
        return 0;

    // write 64bit float value
    yabe_poke_int8( cursor, yabe_flt64_tag);
    yabe_poke_int64( cursor, dr );
    cursor->len -= len;
    return len;
}

/* Return 0 if could write the utf8 string size at cursor position,
   otherwise return the number of missing bytes in buffer */
size_t yabe_write_string( yabe_cursor_t* cursor, size_t strLen )
{
    if( strLen < 64 )
    {
        const size_t len = sizeof(int8_t);
        if( cursor->len < len )
            return 0;
        yabe_poke_int8( cursor, yabe_str6_tag | (uint8_t)strLen );
        cursor->len -= len;
        return len;
    }

    if( strLen < 65536 )
    {
        const size_t len = sizeof(int8_t) + sizeof(int16_t);
        if( cursor->len < len )
            return 0;
        yabe_poke_int8( cursor, yabe_str16_tag );
        yabe_poke_uint16( cursor, (uint16_t)strLen );
        cursor->len -= len;
        return len;
    }

    if( strLen < (0x1ULL<<32) )
    {
        const size_t len = sizeof(int8_t) + sizeof(int32_t);
        if( cursor->len < len )
            return 0;
        yabe_poke_int8( cursor, yabe_str16_tag );
        yabe_poke_uint32( cursor, (uint32_t)strLen );
        cursor->len -= len;
        return len;
    }

    const size_t len = sizeof(int8_t) + sizeof(int64_t);
    if( cursor->len < len )
        return 0;
    yabe_poke_int8( cursor, yabe_str16_tag );
    yabe_poke_uint64( cursor, (uint64_t)strLen );
    cursor->len -= len;
    return len;
}


/* Try reading a value as an integer and return the number of byte read */
size_t yabe_read_integer( yabe_cursor_t* cursor, int64_t* value )
{
    int8_t tag = yabe_peek_tag( cursor );
    if( tag >= -32 )
    {
        *value = tag;
        return yabe_skip_tag( cursor );
    }
    if( tag == yabe_int16_tag )
    {
        const size_t len = sizeof(int8_t) + sizeof(int16_t);
        if( cursor->len < len )
            return 0;
        cursor->ptr += sizeof(int8_t);
        *value = *((int16_t*)cursor->ptr);
        cursor->ptr += sizeof(int16_t);
        cursor->len -= len;
        return len;
    }
    if( tag == yabe_int32_tag )
    {
        const size_t len = sizeof(int8_t) + sizeof(int32_t);
        if( cursor->len < len )
            return 0;
        cursor->ptr += sizeof(int8_t);
        *value = *((int32_t*)cursor->ptr);
        cursor->ptr += sizeof(int32_t);
        cursor->len -= len;
        return len;
    }
    if( tag == yabe_int64_tag )
    {
        const size_t len = sizeof(int8_t) + sizeof(int64_t);
        if( cursor->len < len )
            return 0;
        cursor->ptr += sizeof(int8_t);
        *value = *((int64_t*)cursor->ptr);
        cursor->ptr += sizeof(int64_t);
        cursor->len -= len;
        return len;
    }
    return 0;
}


/* Try reading a value as a double float and return the number of byte read */
size_t yabe_read_float( yabe_cursor_t* cursor, double* value )
{
    int8_t tag = yabe_peek_tag( cursor );
    if( tag == yabe_flt0_tag )
    {
        const size_t len = sizeof(int8_t);
        if( cursor->len < len )
            return 0;
        cursor->ptr += sizeof(int8_t);
        *value = 0.;
        cursor->len -= len;
        return len;
    }
    if( tag == yabe_flt16_tag )
    {
        const size_t len = sizeof(int8_t) + sizeof(int16_t);
        if( cursor->len < len )
            return 0;
        cursor->ptr += sizeof(int8_t);
        uint16_t hr = *((uint16_t*)cursor->ptr);
        cursor->ptr += sizeof(uint16_t);
        int16_t he = hr&0x7C00;             // get exponent bits of half float
        uint64_t dr;
        if( he == 0x7C00 )
        {
            if( hr&0x3FF )
                dr = 0x7FF4000000000000LL;  // normalized NaN
            else if( hr&0x8000 )
                dr = 0xFFF0000000000000LL;  // - infinity
            else
                dr = 0x7FF0000000000000LL;  // + infinity
        }
        else
        {
            dr = (he >> 10)-15+1023;          // set value exponent bits
            dr <<= 52;
            if( hr & 0x8000 ) dr |= (1LL<<63); // set value sign bit
            dr |= ((uint64_t)(hr & 0x3FF)) << (52-10);     // set value mantissa
        }
        // get float value as int64 value
        uint64_t* rPtr = &dr;
        *value = *((double*)(rPtr));              // assign value as double float
        cursor->len -= len;
        return len;
    }
    if( tag == yabe_flt32_tag )
    {
        const size_t len = sizeof(int8_t) + sizeof(int32_t);
        if( cursor->len < len )
            return 0;
        cursor->ptr += sizeof(int8_t);
        *value = *((float*)cursor->ptr);
        cursor->ptr += sizeof(uint32_t);
        cursor->len -= len;
        return len;
    }
    if( tag == yabe_flt64_tag )
    {
        const size_t len = sizeof(int8_t) + sizeof(int64_t);
        if( cursor->len < len )
            return 0;
        cursor->ptr += sizeof(int8_t);
        *value = *((double*)cursor->ptr);
        cursor->ptr += sizeof(uint64_t);
        cursor->len -= len;
        return len;
    }
    return 0;
}

/* Try reading a value as a string and return the number of byte read */
size_t yabe_read_string( yabe_cursor_t* cursor, size_t* length )
{
    int8_t tag = yabe_peek_tag( cursor );
    if( (tag & ((int8_t)-64)) == yabe_str6_tag )
    {
        const size_t len = sizeof(int8_t);
        if( cursor->len < len )
            return 0;
        cursor->ptr += sizeof(int8_t);
        *length = tag & (int8_t)0x3F;
        cursor->len -= len;
        return len;
    }
    if( tag == yabe_str16_tag )
    {
        const size_t len = sizeof(int8_t) + sizeof(uint16_t);
        if( cursor->len < len )
            return 0;
        cursor->ptr += sizeof(int8_t);
        *length = *((uint16_t*)cursor->ptr);
        cursor->ptr += sizeof(uint16_t);
        cursor->len -= len;
        return len;
    }
    if( tag == yabe_str32_tag )
    {
        const size_t len = sizeof(int8_t) + sizeof(uint32_t);
        if( cursor->len < len )
            return 0;
        cursor->ptr += sizeof(int8_t);
        *length = *((uint32_t*)cursor->ptr);
        cursor->ptr += sizeof(uint32_t);
        cursor->len -= len;
        return len;
    }
    else if( tag == yabe_str64_tag )
    {
        const size_t len = sizeof(int8_t) + sizeof(uint64_t);
        if( cursor->len < len )
            return 0;
        cursor->ptr += sizeof(int8_t);
        *length = *((uint64_t*)cursor->ptr);
        cursor->ptr += sizeof(uint64_t);
        cursor->len -= len;
        return len;
    }
    return 0;
}

