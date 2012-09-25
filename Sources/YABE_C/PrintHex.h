#ifndef PRINTHEX_H
#define PRINTHEX_H


// Display data in hexadecimal
inline void printHex( const void* data, size_t len,  const char* margin )
{
    if( len == 0 )
    {
        if( margin )
            printf( "%s", margin );
        printf("\n");
        return;
    }
    int x = 0;
    size_t cnt = 0;
    char ascii[32], *ap;
    const unsigned char* p = (unsigned char*)data;
    while( len-- )
    {
        if( x == 0 )
        {
            if( margin )
                printf( "%s", margin );
            printf( "%6.06lld ", (unsigned long long)cnt );
            ap = ascii;
        }

        if( x++ == 8 )
            printf( " " );

        printf( "%02X ", *p );

        *ap = ( *p >= ' ' && *p < 127 ) ? *p : '.';

        if( x == 16 )
        {
            *ap = '\0';
            printf( " %s\n", ascii );
            x = 0;
        }
        p++;
        ap++;
        cnt++;
    }

    if( x > 0 )
    {
        if( x < 8 )
            printf( " " );
        while( x++ < 16 )
            printf( "   " );
        *ap = '\0';
        printf( " %s\n", ascii );
    }
    if( margin )
        printf( "%s", margin );
    printf( "%6.06lu\n", cnt );
}



#endif // PRINTHEX_H
