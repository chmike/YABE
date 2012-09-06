YABE 
====

YABE is **Y**et **A**nother **B**inary **E**ncoding proposal as simple as JSON, but with the addition of a binary blob data value (i.e. Images) and the benefit of marshalling binary data.

Atomic data values :

0. *null* ;
1. **Boolean** : *true* & *false* ;
2. **Integer** : 64 bit integer ;
3. **Floating point** : 64 bit IEEE 754-2008 ;
4. **String** : utf8 encoded character sequence ;
5. **Blob** : array of raw bytes with a mime type string ;

Composed values :

1. **Array** : Sequence of any values ;
2. **Object** : Sequence of a pair of value identifier (string) and any value ; 

Value Encoding 
--------------

Each value is encoded as a tag byte identifying its type, followed by an optional value size if it is not implicit or stored in the tag, and the value itself if it couldn't be stored in the tag : 

    [tag]([size])([value]) 

* The tag is a one byte value but may later be extended to a multibyte value if required ;
* The value size, when present, is encoded as a 16 bit or 32bit little endian unsigned integer ;
* The value has a type specific encoding using always the most compact form :
    * All multi byte integers are encoded in little endian order ;
    * Floating point values are encoded in the IEEE 754-2008 format ;
* An Array or Object is a stream of values ; 
* A string is a byte array of utf8 encoded characters ;
* A blob is a mime type string followed by an array of raw bytes ;

Table of Tags
-------------

    #tags |   Value type     |    Tag    | arguments                 | comment
	-------------------------------------------------------------------------------------------
    128    small integers >0 : [0xxxxxxx]                            : 0..127 value encoded as is in tag
     32    small integers <0 : [111xxxxx]                            : -1..-32 value encoded as is in tag
     64    short strings     : [10xxxxxx] [utf8 bytes]*              : 0 to 63 byte long utf8 string
     32    ...               : [110xxxxx]                            : 32 tag values listed beneath
        1    ignore          : [11000000]                            : tag byte to be ignored
        1    null            : [11000001]                            : null value 
        1    boolean         : [11000010]                            : false value
        1    boolean         : [11000011]                            : true value
        1    end             : [11000100]                            : equivalent to ] or }
        1    integer         : [11000101] [2 byte]                   : 16 bit integer
        1    integer         : [11000110] [4 byte]                   : 32 bit integer
        1    integer         : [11000111] [8 byte]                   : 64 bit integer
        1    floating point  : [11001000]                            : 0. float value
        1    floating point  : [11001001] [2 byte]                   : 16 bit float
        1    floating point  : [11001010] [4 byte]                   : 32 bit float
        1    floating point  : [11001011] [8 byte]                   : 64 bit float
        1    len16 string    : [11001100] [2 byte] [utf8 bytes]*     : string of utf8 chars
        1    len32 string    : [11001101] [4 byte] [utf8 bytes]*     : string of utf8 chars
        1    len16 blob      : [11001110] [string] [2 byte] [bytes]* : mime typed byte array
        1    len32 blob      : [11001111] [string] [4 byte] [bytes]* : mime typed byte array
        7    array           : [11010xxx] [value]*                   : 0 to 6 value array
        1    array start     : [11010111] [value]*                   : equivalent to [
        7    object          : [11011xxx] [string, value]*           : 0 to 6 pairs of identifier and value
        1    object start    : [11011111] [string, value]*           : equivalent to {


YABE data Signature
-------------------

Saved or transmitted YABE encoded data starts with a five byte signature. The first four bytes are the ASCII code 'Y', 'A', 'B', 'E' in that order, and the fifth byte is the version number of the encoding which is currently 0. The encoded data byte length is defined by the context (i.e. file or record size).

File extension and mime type
----------------------------

A file name with extention .yabe and starting with the YABE tag contains YABE encoded data. The mime type is "application/yabe" (to be registered).

