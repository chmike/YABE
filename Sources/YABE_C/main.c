#include <stdio.h>
#include <stdlib.h>

#include "yabe.h"
#include "PrintHex.h"


/* Byte order swaping: 1234578 -> 87654321 */
inline uint64_t byte_swap_uint64( uint64_t val )
{
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL ) | ((val >> 8) & 0x00FF00FF00FF00FFULL );
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL ) | ((val >> 16) & 0x0000FFFF0000FFFFULL );
    return (val << 32) | (val >> 32);
}


int main(void)
{

    /*
    uint64_t val1 = 0x1122334455667788ULL;
    uint64_t val2 = byte_swap_uint64( val1 );

    printf( "%16.016llX -> %16.016llX\n", (unsigned long long)val1, (unsigned long long)val2 );
    exit(0);
    */
    // Buffer
    const size_t bufLen = 1024*1024;
    char buffer[bufLen];
    size_t res;
    // Clear buffer
    memset( buffer, 0, bufLen );

    // Initialise writing cursor
    yabe_cursor_t wCurInit = { buffer, bufLen }, wCur = wCurInit;

    // Initialize reading cursor
    yabe_cursor_t rCurInit = { buffer, 0 }, rCur = rCurInit;

    rCur.len += yabe_write_null( &wCur );
    if( !yabe_read_null( &rCur ) )
    {
        printf( "Failed reading null\n" );
        exit(1);
    }
    rCur = rCurInit; wCur = wCurInit;

    int64_t wInteger, rInteger;
    double wFloat, rFloat;
    wInteger = 100;
    rInteger = 0;
    rCur.len += yabe_write_integer( &wCur, wInteger );
    res = yabe_read_integer( &rCur, &rInteger);
    if( !res || rInteger != wInteger )
    {
        printf( "Failed reading integer %lld\n", (long long)wInteger );
        exit(1);
    }
    rCur = rCurInit; wCur = wCurInit;

    wInteger = 0x7FFF;
    rInteger = 0;
    rCur.len += yabe_write_integer( &wCur, wInteger );
    res = yabe_read_integer( &rCur, &rInteger);
    if( !res || rInteger != wInteger )
    {
        printf( "Failed reading integer %lld\n", (long long)wInteger );
        exit(1);
    }
    rCur = rCurInit; wCur = wCurInit;

    wInteger = 0x7FFFFFFF;
    rInteger = 0;
    rCur.len += yabe_write_integer( &wCur, wInteger );
    res = yabe_read_integer( &rCur, &rInteger);
    if( !res || rInteger != wInteger )
    {
        printf( "Failed reading integer %lld\n", (long long)wInteger );
        exit(1);
    }
    rCur = rCurInit; wCur = wCurInit;

    wInteger = 1LL<<32;
    rInteger = 0;
    rCur.len += yabe_write_integer( &wCur, wInteger );
    res = yabe_read_integer( &rCur, &rInteger);
    if( !res || rInteger != wInteger )
    {
        printf( "Failed reading integer %lld\n", (long long)wInteger );
        exit(1);
    }
    rCur = rCurInit; wCur = wCurInit;

    wFloat = 0.;
    rFloat = 1.;
    rCur.len += yabe_write_float( &wCur, wFloat );
    res = yabe_read_float( &rCur, &rFloat);
    if( !res || rFloat != wFloat )
    {
        printf( "Failed reading float %G, got %G instead\n", wFloat, rFloat );
        exit(1);
    }
    rCur = rCurInit; wCur = wCurInit;

    wFloat = -0.;
    rFloat = 1.;
    rCur.len += yabe_write_float( &wCur, wFloat );
    res = yabe_read_float( &rCur, &rFloat);
    if( !res || rFloat != wFloat )
    {
        printf( "Failed reading float %G, got %G instead\n", wFloat, rFloat );
        exit(1);
    }
    rCur = rCurInit; wCur = wCurInit;

    wFloat = 4.5;
    rFloat = 0.;
    rCur.len += yabe_write_float( &wCur, wFloat );
    res = yabe_read_float( &rCur, &rFloat);
    if( !res || rFloat != wFloat )
    {
        printf( "Failed reading float %G, got %G instead\n", wFloat, rFloat );
        exit(1);
    }
    rCur = rCurInit; wCur = wCurInit;

    wFloat = -4.5;
    rFloat = 0.;
    rCur.len += yabe_write_float( &wCur, wFloat );
    res = yabe_read_float( &rCur, &rFloat);
    if( !res || rFloat != wFloat )
    {
        printf( "Failed reading float %G, got %G instead\n", wFloat, rFloat );
        exit(1);
    }
    rCur = rCurInit; wCur = wCurInit;

    wFloat = 65537.;
    rFloat = 0.;
    rCur.len += yabe_write_float( &wCur, wFloat );
    res = yabe_read_float( &rCur, &rFloat);
    if( !res || rFloat != wFloat )
    {
        printf( "Failed reading float %G, got %G instead\n", wFloat, rFloat );
        exit(1);
    }
    rCur = rCurInit; wCur = wCurInit;

    wFloat = -65537.;
    rFloat = 0.;
    rCur.len += yabe_write_float( &wCur, wFloat );
    res = yabe_read_float( &rCur, &rFloat);
    if( !res || rFloat != wFloat )
    {
        printf( "Failed reading float %G, got %G instead\n", wFloat, rFloat );
        exit(1);
    }
    rCur = rCurInit; wCur = wCurInit;

    wFloat = 0.128;
    rFloat = 0.;
    rCur.len += yabe_write_float( &wCur, wFloat );
    res = yabe_read_float( &rCur, &rFloat);
    if( !res || rFloat != wFloat )
    {
        printf( "Failed reading float %G, got %G instead\n", wFloat, rFloat );
        exit(1);
    }
    rCur = rCurInit; wCur = wCurInit;

    bool wBool = true;
    bool rBool = false;
    rCur.len += yabe_write_bool( &wCur, wBool );
    res = yabe_read_bool( &rCur, &rBool);
    if( !res || wBool != rBool )
    {
        printf( "Failed reading bool 'true'\n" );
        exit(1);
    }
    rCur = rCurInit; wCur = wCurInit;

    char wString[1024], rString[1024];
    strcpy( wString, "short string" );
    strcpy( rString, "" );
    size_t wStrLen = strlen( wString )+1, rStrLen = 0;

    rCur.len += res = yabe_write_string( &wCur, wStrLen );
    if( res )
        rCur.len += yabe_write_data( &wCur, wString, wStrLen );
    res = yabe_read_string( &rCur, &rStrLen );
    if( !res || !strcmp( wString, rString ) )
    {
        printf( "Failed reading string length %lld\n", (unsigned long long)wStrLen );
        exit(1);
    }
    res = yabe_read_data( &rCur, rString, rStrLen );
    if( !res )
    {
        printf( "Failed reading string data\n" );
        exit(1);
    }
    rCur = rCurInit; wCur = wCurInit;

    memset( wString, 'A', 80 );
    wString[80] = '\0';
    strcpy( rString, "" );
    wStrLen = strlen( wString )+1, rStrLen = 0;

    rCur.len += res = yabe_write_string( &wCur, wStrLen );
    if( res )
        rCur.len += yabe_write_data( &wCur, wString, wStrLen );
    // printHex( rCur.ptr, rCur.len, 0 );
    res = yabe_read_string( &rCur, &rStrLen );
    if( !res || !strcmp( wString, rString ) )
    {
        printf( "Failed reading string length %lld\n", (unsigned long long)wStrLen );
        exit(1);
    }
    res = yabe_read_data( &rCur, rString, rStrLen );
    if( !res )
    {
        printf( "Failed reading string data\n" );
        exit(1);
    }
    rCur = rCurInit; wCur = wCurInit;

    /* All other functions and encoding should work as expected */

    printf("Done!\n");
    return 0;
}

