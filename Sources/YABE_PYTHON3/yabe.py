'''
NAME
    yabe - YABE serialization of Python objects.

DESCRIPTION
    This module provides serialization/deserialization of objects using the 
    YABE (Yet Another Binary Encoding) format. This module provides the same 
    interface as the standard pickle module traditionally used for serialization
    of Python objects.
'''

import codecs
import collections
import io
import random
import struct
import types

# YABE type identifier constants

STR6    = 0x80
NULL    = 0xC0
INT16   = 0xC1
INT32   = 0xC2
INT64   = 0xC3
FLT0    = 0xC4
FLT16   = 0xC5
FLT32   = 0xC6
FLT64   = 0xC7
FALSE   = 0xC8
TRUE    = 0xC9
BLOB    = 0xCA
ENDS    = 0xCB
STR16   = 0xCD
STR32   = 0xCE
STR64   = 0xCF
SARRAY  = 0xD0
ARRAY   = 0xD7
SOBJECT = 0xD8
OBJECT  = 0xDF

def _encode(obj, dest):
    if obj is None:
        _encodeNone(dest)
    elif type(obj) == int:
        _encodeInteger(obj, dest)
    elif type(obj) == float:
        _encodeFloat(obj, dest)
    elif type(obj) == bool:
        _encodeBoolean(obj, dest)
    elif type(obj) == str:
        _encodeString(obj, dest)
    elif isinstance(obj, collections.Iterable):
        _encodeIterable(obj, dest)
    else:
        _encodeObject(obj, dest)
    
     
def _encodeInteger(obj: int, dest):
    assert type(obj) == int

    if (-32 <= obj <= 127):
        dest.write(struct.pack('b', obj))
    elif -32768 <= obj <= 32767:
        dest.write(struct.pack('<Bh', INT16, obj))
    elif -2147483648 <= obj <= 2147483647:
        dest.write(struct.pack('<Bi', INT32, obj))
    elif -9223372036854775808 <= obj <= 9223372036854775807:
        dest.write(struct.pack('<Bq', INT64, obj))
    else:
        raise ValueError('Cannot serialize integers larger than 64 bits in YABE')


def _encodeBoolean(obj: bool, dest):
    assert type(obj) == bool

    if obj:
        dest.write(struct.pack('B', TRUE))
    else:
        dest.write(struct.pack('B', FALSE))


def _encodeFloat(obj: float, dest):
    assert type(obj) == float
    if obj == 0:
        dest.write(struct.pack('B', FLT0))
    else:
        EXPONENT_BITS =  0x7FF << 52

        # get float value as int representation
        dr = struct.unpack('q', struct.pack('d', obj))[0]
        
        # get exponent bits from int value and clear sign bit
        de = dr & EXPONENT_BITS

        # if value is infinity or NaN, write as flt16
        if de == EXPONENT_BITS:
            if dr & 0xFFFFFFFFFFFFF:
                hr = 0x7D00 # NaN
            elif dr < 0:
                hr = 0xFC00 # -inf
            else:
                hr = 0x7C00 # +inf
            dest.write(struct.pack('<Bh', FLT16, hr))
            return

        # get exponent value
        he = (de >> 52) - 1023

        # if value fits in a flt16
        if (-14 <= he <= 15) and ((dr & 0x3FFFFFFFFFF) == 0):
            hr = (he + 15) << 10        # exponent
            if dr < 0:
                hr |= 0x8000            # sign
            hr |= ((dr >> 42) & 0x3FF)  # mantissa
            dest.write(struct.pack('<Bh', FLT16, hr))

        # if value fits in a flt32
        elif (-126 <= he <= 127) and ((dr & 0x1FFFFFFF) == 0):
            fr = (he + 127) << 23
            if dr < 0:
                fr |= 0x80000000
            fr |= (dr >> 29) & 0x7FFFFF
            dest.write(struct.pack('<Bi', FLT32, fr))

        else:
            dest.write(struct.pack('<Bd', FLT64, obj))


def _encodeBytes(obj: bytes, mimetype: str, dest):
    assert type(mimetype) == str
    assert type(obj) == bytes
    dest.write(struct.pack('B', BLOB) + _encodeString(mimetype))
    l = len(obj)
    if l <= 63:
        code = 128 + l
        dest.write(struct.pack('B', code) + obj)
    elif l <= 65535:
        dest.write(struct.pack('<BH', STR16, l) + obj)
    elif l < 2**32:
        dest.write(struct.pack('<BI', STR32, l) + obj)
    else:
        dest.write(struct.pack('<BQ', STR64, l) + obj)


def _encodeString(obj: str, dest):
    assert type(obj) == str
    u8chars = codecs.encode(obj, 'utf-8')
    l = len(u8chars)
    if l <= 63:
        code = STR6 + l
        dest.write(struct.pack('B', code) + u8chars)
    elif l <= 65535:
        dest.write(struct.pack('<BH', STR16, l) + u8chars)
    elif l < 2**32:
        dest.write(struct.pack('<BI', STR32, l) + u8chars)
    else:
        dest.write(struct.pack('<BQ', STR64, l) + u8chars)


def _encodeIterable(obj, dest):
    l = len(obj)
    if l < 7:
        dest.write(struct.pack('B', SARRAY + l))
        for element in obj:
            _encode(element, dest)
    else:
        dest.write(struct.pack('B', ARRAY))
        for element in obj:
            _encode(element, dest)
        dest.write(struct.pack('B', ENDS))


def _encodeNone(dest):
    dest.write(struct.pack('B', NULL))


def _encodeObject(obj, dest):
    fields = [field for field in dir(obj) 
                if not isinstance(field, (types.MethodType, 
                    types.BuiltinFunctionType, types.BuiltinMethodType, 
                    types.CodeType, types.CodeType, types.FrameType, 
                    types.FunctionType, types.GeneratorType, 
                    types.GetSetDescriptorType, types.MappingProxyType, 
                    types.MemberDescriptorType, types.ModuleType))
                and field[:2] != '__'
    ]
    l = len(fields)
    if l <= 5:
        dest.write(struct.pack('B', 0xD8 + l))
        for field in fields:
            _encodeString(field, dest)
            try:
                _encode(getattr(obj, field), dest)
            except AttributeError:
                _encodeNone(dest)
    else:
        dest.write(struct.pack('B', OBJECT))
        for field in fields:
            try:
                value = getattr(obj, field)
                _encodeString(field, dest)
                _encode(value, dest)
            except AttributeError:
                pass
        dest.write(struct.pack('B', ENDS))


def _decodeInteger(tag, f) -> object:
    assert tag in (INT16, INT32, INT64)

    params = {INT16: '<h', INT32: '<i', INT64: '<q'}
    fmt = params[tag]
    size = struct.calcsize(fmt)
    bytes = f.read(size)
    if len(bytes) < size:
        raise IOError('Incomplete YABE sequence')
    return struct.unpack(fmt, b)[0]


def _decodeFloat(tag, f) -> object:
    assert tag in (FLT16, FLT32, FLT64)

    params = {FLT16: '<h', FLT32: '<f', FLT64: '<d'}
    fmt = params[tag]
    size = struct.calcsize(fmt)
    bytes = f.read(size)
    if len(bytes) < size:
        raise IOError('Incomplete YABE sequence')
    if tag == FLT16:
        hr = struct.unpack(fmt, bytes)[0]
        he = hr & 0x7C00
        dr = 0
        if he == 0x7C00:
            if hr & 0x3FF:
                dr = 0x7FF4000000000000 # normalized NaN
            elif hr & 0x8000:
                dr = 0xFFF0000000000000 # -?
            else:
                dr = 0x7FF0000000000000 # +?
        else:
            dr = (he >> 10) - 15 + 1023 # set value exponent bits
            dr <<= 52
            if hr & 0x8000:
                dr |= (1 << 63)
            dr |= (hr & 0x3FF) << 42 # set value mantissa
        return struct.unpack('<d', struct.pack('<q', dr))[0]
    else:
        return struct.unpack(fmt, bytes)[0]


def _decodeString(tag, f):
    assert tag in (STR16, STR32, STR64) or ((tag & 0xC0) == STR6)

    if (tag & 0xC0) == STR6:
        l = tag & 0x3F
    else:
        params = {STR16: '<h', STR32: '<i', STR64: '<q'}
        fmt = params[tag]
        size = struct.calcsize(fmt)
        bytes = f.read(size)
        if len(bytes) < size: 
            raise IOError('Incomplete YABE sequence')
        l = struct.unpack(fmt, bytes)[0]
    u8s = f.read(l)
    if len(u8s) < l:
        raise IOError('Incomplete YABE sequence')
    return codecs.decode(u8s, 'utf-8')


def _decodeArray(f):
    ls = list()
    obj = _decode(f)
    while obj != ENDS:
        ls.append(obj)
        obj = _decode(f)
    return ls


def _decodeShortArray(tag, f):
    assert (tag & 0xF8) == 0xD0

    size = tag & 7
    ls = list()
    for i in range(size):
        obj = _decode(f)
        ls.append(obj)
    return ls


def _decodeBlob(f):
    byte = f.read(1)
    if not len(byte):
        raise IOError('Incomplete YABE sequence')
    tag = struct.unpack('B', byte)[0]
    mime = _decodeString(tag, f)
    byte = f.read(1)
    if not len(byte):
        raise IOError('Incomplete YABE sequence')
    tag = struct.unpack('B', byte)[0]
    if (tag & 0xC0) == STR6:
        l = (tag & 0x3F)
    else:
        params = {STR16: ('<h', 2), STR32: ('<i', 4), STR64: ('<q', 8)}
        fmt, size = params[tag]
        bytes = f.read(size)
        if len(bytes) < size: 
            raise IOError('Incomplete YABE sequence')
        l = struct.unpack(fmt, bytes)[0]
    bytes = f.read(l)
    if len(u8s) < l:
        raise IOError('Incomplete YABE sequence')
    return (mime, bytes)


def _decodeObject(f):
    # We need an empty class and an instance of this class
    class YabeObject:
        pass
    obj = YabeObject()

    fieldName = _decode(f)
    while fieldName != ENDS:
        field = _decode(f)
        obj.__setattr__(fieldName, field)
        fieldName = _decode(f)
    return obj


def _decodeShortObject(f, tag):
    class YabeObject:
        pass
    obj = YabeObject()

    nbFields = (tag & 7)
    for i in range(nbFields):
        fieldName = _decode(f)
        if type(fieldName) != str:
            raise TypeError('Was expecting a field name as a string')
        field = _decode(f)
        obj.__setattr__(fieldName, field)
    return obj


def _decode(f) -> object:
    c = f.read(1)
    if len(c) == 0:
        raise IOrror('Incomplete YABE sequence')

    tag = struct.unpack('B', c)[0]
    if 0 <= tag <= 127: 
        return tag
    elif 224 <= tag <= 255:
        return tag - 256
    elif tag in (INT16, INT32, INT64):
        return _decodeInteger(tag, f)
    elif tag == FLT0:
        return 0.0
    elif tag in (FLT16, FLT32, FLT64):
        return _decodeFloat(tag, f)
    elif (tag in (STR16, STR32, STR64)) or ((tag & 0xC0) == 0x80):
        return _decodeString(tag, f)
    elif tag == TRUE:
        return True
    elif tag == FALSE:
        return False
    elif tag == ARRAY:
        return _decodeArray(f)
    elif (tag & 0xF8) == SARRAY:
        return _decodeShortArray(tag, f)
    elif tag == BLOB:
        return _decodeBlob(f)
    elif tag == OBJECT:
        return _decodeObject(f)
    elif (tag & 0xF8) == SOBJECT:
        return _decodeShortObject(f, tag)
    elif tag == NULL:
        return None
    elif tag == ENDS:
        return ENDS


def dump(obj, f, protocol=0):
    '''
    Serializes an object using a file-like object as destination.
    Parameters:
     - obj:      The object to serialize.
     - f:        A file-like object data will be written to.
     - protocol: reserved for future use. This parameter should be 0.
    '''
    if protocol != 0:
        raise ValueError('Unsupported protocol version')
    f.write(b'YABE')
    f.write(struct.pack('B', protocol))
    _encode(obj, f)

     
def dumps(obj, protocol=0) -> bytes:
    with io.BytesIO() as f:
        dump(obj, f, protocol)
        return f.getvalue()    


def load(f) -> object:
    signature = f.read(5)
    if len(signature) < 5 or signature[:4] != b'YABE':
        raise ValueError('Not a YABE stream (incorrect signature)')
    version = struct.unpack('B', signature[4:])[0]
    if version > 0:
        raise ValueError('Yabe version not supported')
    return _decode(f)


def loads(b) -> object:
    with io.BytesIO(b) as f:
        return load(f)

def _unittests():
    print('Testing tiny integers')
    for i in range(0, 128, 1):
        with io.BytesIO() as f:
            _encodeInteger(i, f)
            b = f.getvalue()
            assert len(b) == 1
            assert ord(b) == i

    for i in range(-32, 0, 1):
        with  io.BytesIO() as f:
            _encodeInteger(i, f)
            b = f.getvalue()
            assert len(b) == 1
            assert (ord(b) - 256) == i
            
    print('Testing short integers')
    for i in range(128, 32768, 1):
        with io.BytesIO() as f:
            _encodeInteger(i, f)
            b = f.getvalue()
            assert len(b) == 3
            assert b[0] == INT16
            assert b[1] == i & 0xFF
            assert b[2] == i >> 8

    print('Testing integers')
    for times in range(10000):
        i = random.randint(32768, 2**31)
        with io.BytesIO() as f:
            _encodeInteger(i, f)
            b = f.getvalue()
            assert len(b) == 5
            assert b[0] == INT32
            assert b[1] == i & 0xFF
            assert b[2] == (i >> 8) & 0xFF
            assert b[3] == (i >> 16) & 0xFF
            assert b[4] == (i >> 24) & 0xFF

    print('Testing long integers') 
    for times in range(10000):
        i = random.randint(2**32, 2**63)
        with io.BytesIO() as f:
            _encodeInteger(i, f)
            b = f.getvalue()
            assert len(b) == 9
            assert b[0] == INT64
            assert b[1] == i & 0xFF
            assert b[2] == (i >> 8) & 0xFF 
            assert b[3] == (i >> 16) & 0xFF
            assert b[4] == (i >> 24) & 0xFF
            assert b[5] == (i >> 32) & 0xFF
            assert b[6] == (i >> 40) & 0xFF
            assert b[7] == (i >> 48) & 0xFF
            assert b[8] == (i >> 56) & 0xFF

    print('Testing floating point values')
    for times in range(10000):
        d = random.random()
        with io.BytesIO() as f:
            _encodeFloat(d, f)
            b = f.getvalue()
            assert len(b) in (1, 3, 5, 9)
            assert b[0] in (FLT0, FLT16, FLT32, FLT64)
            with io.BytesIO(b[1:]) as f2:
                d2 = _decodeFloat(b[0], f2)
                assert d2 == d

    print('Testing arrays')
    nb = random.randint(1, 10)
    l = list()
    for n in range(nb):
        l.append(random.randint(0, 100))
    with io.BytesIO() as f:
        _encodeIterable(l, f)
        b = f.getvalue()
        assert (b[0] == ARRAY) or ((b[0] & 0xF8) == 0xD0)
        with io.BytesIO(b) as f2:
            l2 = _decode(f2)
            assert l == l2

    print('Testing Strings')
    nb = random.randint(1, 65536)
    l = list()
    for n in range(nb):
        l.append(random.choice('abcdefghijklmnopqrstuvxyz'))
    s = ''.join(l)
    with io.BytesIO() as f:
        _encodeString(s, f)
        b = f.getvalue()
        with io.BytesIO(b) as f2:
            s2 = _decode(f2)
            assert s == s2

    print('Testing objects and public interface')
    class C:
        def __init__(self):
            self.a = 1
            self.b = 2.0
            self.c = "three"
            self.d = [1, 2.0, 'three', 4]

    c = C()
    b = dumps(c)
    c2 = loads(b)

    assert c2.a == c.a
    assert c2.b == c.b
    assert c2.c == c.c
    assert c2.d == c.d


if __name__ == '__main__':
    _unittests()
