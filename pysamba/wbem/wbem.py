from pysamba.library import *
from pysamba.composite_context import composite_context

class com_context(Structure): pass

CIMSTRING = c_char_p
CIM_EMPTY=0
CIM_SINT8=16
CIM_UINT8=17
CIM_SINT16=2
CIM_UINT16=18
CIM_SINT32=3
CIM_UINT32=19
CIM_SINT64=20
CIM_UINT64=21
CIM_REAL32=4
CIM_REAL64=5
CIM_BOOLEAN=11
CIM_STRING=8
CIM_DATETIME=101
CIM_REFERENCE=102
CIM_CHAR16=103
CIM_OBJECT=13
CIM_FLAG_ARRAY=0x2000
CIM_ARR_SINT8=CIM_FLAG_ARRAY|CIM_SINT8
CIM_ARR_UINT8=CIM_FLAG_ARRAY|CIM_UINT8
CIM_ARR_SINT16=CIM_FLAG_ARRAY|CIM_SINT16
CIM_ARR_UINT16=CIM_FLAG_ARRAY|CIM_UINT16
CIM_ARR_SINT32=CIM_FLAG_ARRAY|CIM_SINT32
CIM_ARR_UINT32=CIM_FLAG_ARRAY|CIM_UINT32
CIM_ARR_SINT64=CIM_FLAG_ARRAY|CIM_SINT64
CIM_ARR_UINT64=CIM_FLAG_ARRAY|CIM_UINT64
CIM_ARR_REAL32=CIM_FLAG_ARRAY|CIM_REAL32
CIM_ARR_REAL64=CIM_FLAG_ARRAY|CIM_REAL64
CIM_ARR_BOOLEAN=CIM_FLAG_ARRAY|CIM_BOOLEAN
CIM_ARR_STRING=CIM_FLAG_ARRAY|CIM_STRING
CIM_ARR_DATETIME=CIM_FLAG_ARRAY|CIM_DATETIME
CIM_ARR_REFERENCE=CIM_FLAG_ARRAY|CIM_REFERENCE
CIM_ARR_CHAR16=CIM_FLAG_ARRAY|CIM_CHAR16
CIM_ARR_OBJECT=CIM_FLAG_ARRAY|CIM_OBJECT
CIM_ILLEGAL=0xfff
CIM_TYPEMASK=0x2FFF

class CIMSTRINGS(Structure):
    _fields_ = [
        ('count', uint32_t),
        ('item', POINTER(CIMSTRING))
        ]

class WbemClassObject(Structure): pass

class arr_int8(Structure):
    _fields_= [
        ('count', uint32_t),
        ('item', POINTER(int8_t))
        ]
class arr_uint8(Structure):
    _fields_= [
        ('count', uint32_t),
        ('item', POINTER(uint8_t))
        ]
class arr_int16(Structure):
    _fields_= [
        ('count', uint32_t),
        ('item', POINTER(int16_t))
        ]
class uarr_uint16(Structure):
    _fields_= [
        ('count', uint32_t),
        ('item', POINTER(uint16_t))
        ]
class arr_int32(Structure):
    _fields_= [
        ('count', uint32_t),
        ('item', POINTER(int32_t))
        ]
class arr_uint32(Structure):
    _fields_= [
        ('count', uint32_t),
        ('item', POINTER(uint32_t))
        ]
dlong_t = int64_t
class arr_dlong(Structure):
    _fields_= [
        ('count', uint32_t),
        ('item', POINTER(dlong_t))
        ]
class arr_uint32(Structure):
    _fields_= [
        ('count', uint32_t),
        ('item', POINTER(uint32_t))
        ]
udlong_t = uint64_t
class arr_udlong(Structure):
    _fields_= [
        ('count', uint32_t),
        ('item', POINTER(udlong_t))
        ]
class arr_uint16(Structure):
    _fields_= [
        ('count', uint32_t),
        ('item', POINTER(uint16_t))
        ]
class arr_CIMSTRING(Structure):
    _fields_= [
        ('count', uint32_t),
        ('item', POINTER(CIMSTRING))
        ]
class arr_WbemClassObject(Structure):
    _fields_= [
        ('count', uint32_t),
        ('item', POINTER(WbemClassObject))
        ]

class CIMVAR(Union):
    _fields_ = [
        ('v_sint8', int8_t),            # case (CIM_SINT8)
        ('v_uint8', uint8_t),           # case (CIM_UINT8)
        ('v_sint16', int16_t),          # case (CIM_SINT16)
        ('v_uint16', uint16_t),         # case (CIM_UINT16)
        ('v_sint32', int32_t),          # case (CIM_SINT32)
        ('v_uint32', uint32_t),         # case (CIM_UINT32)
        ('v_sint64', int64_t),          # case (CIM_SINT64)
        ('v_uint64', uint64_t),         # case (CIM_UINT64)
        ('v_real32', uint32_t),         # case (CIM_REAL32)
        ('v_real64', uint64_t),         # case (CIM_REAL64)
        ('v_boolean', uint16_t),        # case (CIM_BOOLEAN)
        ('v_string', CIMSTRING), # relative,string,charset(UTF16),case(CIM_STRING)]
        ('v_datetime', CIMSTRING), # relative,string,charset(UTF16),case(CIM_DATETIME)]
        ('v_reference', CIMSTRING), # relative,string,charset(UTF16),case(CIM_REFERENCE)]
        ('v_object', POINTER(WbemClassObject)), # [relative,subcontext(4),case(CIM_OBJECT)]
        ('a_sint8', POINTER(arr_int8)), # [relative,case(CIM_ARR_SINT8)] 
        ('a_uint8', POINTER(arr_uint8)), # [relative,case(CIM_ARR_UINT8)] 
        ('a_sint16', POINTER(arr_int16)), # [relative,case(CIM_ARR_SINT16)] 
        ('a_uint16', POINTER(arr_uint16)), # [relative,case(CIM_ARR_UINT16)] 
        ('a_sint32', POINTER(arr_int32)), # [relative,case(CIM_ARR_SINT32)] 
        ('a_uint32', POINTER(arr_uint32)), # [relative,case(CIM_ARR_UINT32)] 
        ('a_sint64', POINTER(arr_dlong)), # [relative,case(CIM_ARR_SINT64)] 
        ('a_uint64', POINTER(arr_udlong)), # [relative,case(CIM_ARR_UINT64)] 
        ('a_real32', POINTER(arr_uint32)), # [relative,case(CIM_ARR_REAL32)] 
        ('a_real64', POINTER(arr_udlong)), # [relative,case(CIM_ARR_REAL64)] 
        ('a_boolean', POINTER(arr_uint16)), # [relative,case(CIM_ARR_BOOLEAN)] 
        ('a_string', POINTER(arr_CIMSTRING)), # [relative,case(CIM_ARR_STRING)] 
        ('a_datetime', POINTER(arr_CIMSTRING)), # [relative,case(CIM_ARR_DATETIME)] 
        ('a_reference', POINTER(arr_CIMSTRING)), # [relative,case(CIM_ARR_REFERENCE)] 
        ('a_object', POINTER(arr_WbemClassObject)), # [relative,case(CIM_ARR_OBJECT)]
        ]


class WbemQualifier(Structure): pass

class WbemQualifiers(Structure):
    _fields_ = [
        ('count', uint32_t),
        ('item',  POINTER(POINTER(WbemQualifier))),
        ]

class WbemPropertyDesc(Structure):
    _fields_ = [
        ('cimtype', uint32_t),
        ('nr', uint16_t),
        ('offset', uint32_t),
        ('depth', uint32_t),
        ('qualifiers', WbemQualifiers),
        ]

class WbemProperty(Structure):
    _fields_ = [
        ('name', CIMSTRING), # [relative,string,charset(UTF16)]
        ('desc', POINTER(WbemPropertyDesc)), # [relative]
        ]
        
class WbemMethods(Structure): pass

class WbemClass(Structure):
    _fields_ = [
        ('u_0', uint8_t),
        ('__CLASS', CIMSTRING),
        ('data_size', uint32_t),
        ('__DERIVATION', CIMSTRINGS),
        ('qualifiers', WbemQualifiers),
        ('__PROPERTY_COUNT', uint32_t),
        ('properties', POINTER(WbemProperty)),
        ('default_flags', POINTER(uint8_t)),
        ('default_values', POINTER(CIMVAR)),
        ]
        
class WbemInstance(Structure):
    _fields_ = [
        ('u1_0', uint8_t),
        ('__CLASS', CIMSTRING),
        ('default_flags', POINTER(uint8_t)),
        ('data', POINTER(CIMVAR)),
        ('u2_4', uint32_t),
        ('u3_1', uint8_t),
        ]

WbemClassObject._fields_ = [
        ('flags', uint8_t),
        ('__SERVER', CIMSTRING),
        ('__NAMESPACE', CIMSTRING),
        ('sup_class', POINTER(WbemClass)),
        ('sup_methods', POINTER(WbemMethods)),
        ('obj_class', POINTER(WbemClass)),
        ('obj_methods', POINTER(WbemMethods)),
        ('instance', POINTER(WbemInstance)),
        ]

class IWbemServices(Structure): pass

WBEM_FLAG_RETURN_IMMEDIATELY = 0x10
WBEM_FLAG_ENSURE_LOCATABLE = 0x100
WBEM_FLAG_FORWARD_ONLY = 0x20

WBEM_S_TIMEDOUT=0x40004

library.WBEM_ConnectServer_send.restype = POINTER(composite_context)
library.WBEM_ConnectServer_recv.restype = WERROR
library.IWbemServices_ExecQuery_send_f.restype = POINTER(composite_context)
library.IWbemServices_ExecQuery_recv.restype = WERROR
library.IEnumWbemClassObject_Reset_send_f.restype = POINTER(composite_context)
library.IEnumWbemClassObject_Reset_recv.restype = WERROR
library.IWbemServices_ExecNotificationQuery_send_f.restype = POINTER(composite_context)
library.IWbemServices_ExecNotificationQuery_recv.restype = WERROR
library.IUnknown_Release_send_f.restype = WERROR
library.IUnknown_Release_recv.restype = POINTER(composite_context)
library.IEnumWbemClassObject_SmartNext_recv.restype = WERROR
library.IEnumWbemClassObject_SmartNext_send.restype = POINTER(composite_context)
