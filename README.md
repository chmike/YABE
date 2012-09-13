# YABE 

YABE is **Y**et **A**nother **B**inary **E**ncoding proposal as simple as JSON, but with the addition of a binary blob data value (i.e. Images) and the benefit of marshalling binary data.

**Author**: Christophe Meessen  
**Date** : 13 sep 2012  


*Atomic data values* :

0. *null* ;
1. **Boolean** : *true* & *false* ;
2. **Integer** : 64 bit integer ;
3. **Floating point** : 64 bit IEEE 754-2008 ;
4. **String** : utf8 encoded character sequence ;
5. **Blob** : array of raw bytes with a mime type string ;

*Composed values* :

1. **Array** : Sequence of any values ;
2. **Object** : Sequence of a pair of value identifier (string) and any value ; 

## Value Encoding 

Each value is encoded as a tag byte identifying its type, followed by an optional value size and the value itself. When possible the size or the value are stored in the tag. 

    [tag]([size])([value]) 

* The tag is a one byte value ;
* The value size, when present, is encoded as a 16 bit, 32bit or 64 bit little endian unsigned integer ;
* The value has a type specific encoding using always the most compact form :
    * All multi byte integers are encoded in little endian order (x86 native representation);
    * Floating point values are encoded in the IEEE 754-2008 format ;
* A string is a byte array of utf8 encoded characters ;
* A blob is an array of raw bytes with a string specifiying its mime type when not empty ;
* An Array is encoded as a stream of values ; 
* An Object is encoded as a stream of string and value pairs ; 

## Table of Tags and encoding

    #tags |   Value type     |    Tag    | arguments                 | comment
    -------------------------------------------------------------------------------------------------------
    128    integers 0..127   : [0xxxxxxx]                            : 0..127 integer value encoded as is
     64    short strings     : [10xxxxxx] [utf8 bytes]*              : 0 to 63 byte long utf8 string
     32                      : [110xxxxx]                            : 32 tag values listed below
         1   null            : [11000000]                            : null value 
         1   integer         : [11000001] [2 byte]                   : 16 bit integer
         1   integer         : [11000010] [4 byte]                   : 32 bit integer
         1   integer         : [11000011] [8 byte]                   : 64 bit integer
         1   floating point  : [11000100]                            : 0. float value
         1   floating point  : [11000101] [2 byte]                   : 16 bit float
         1   floating point  : [11000110] [4 byte]                   : 32 bit float
         1   floating point  : [11000111] [8 byte]                   : 64 bit float
         1   ignore          : [11001000]                            : tag byte to be ignored
         1   len16 string    : [11001001] [2 byte] [utf8 bytes]*     : string of utf8 chars
         1   len32 string    : [11001010] [4 byte] [utf8 bytes]*     : string of utf8 chars
         1   len64 string    : [11001011] [8 byte] [utf8 bytes]*     : string of utf8 chars
         1   stream end      : [11001100]                            : equivalent to ] or }
         1   len16 blob      : [11001101] [2 byte] [string] [bytes]* : mime typed byte array
         1   len32 blob      : [11001110] [4 byte] [string] [bytes]* : mime typed byte array
         1   len64 blob      : [11001111] [8 byte] [string] [bytes]* : mime typed byte array
         7   array           : [11010xxx] [value]*                   : 0 to 6 value array
         1   array stream    : [11010111] [value]*                   : equivalent to [
         7   object          : [11011xxx] [string, value]*           : 0 to 6 pairs of identifier and value
         1   object stream   : [11011111] [string, value]*           : equivalent to {
     32    integers -1..-32  : [111xxxxx]                            : -1..-32 integer value encoded as is


## YABE data size

The encoded data byte length is defined by the context (i.e. file or record size) or is implicit if the data is limited to one value like an array or an object.

## YABE data Signature

Stored or transmitted YABE encoded data starts with a five byte signature. The first four bytes are the ASCII code 'Y', 'A', 'B', 'E' in that order, and the fifth byte is the version number of the encoding. This specification describes the encoding version 0. 

## File extension and mime type

A file name with extention .yabe and starting with the YABE tag contains YABE encoded data. The mime type is "application/yabe" (to be registered).

