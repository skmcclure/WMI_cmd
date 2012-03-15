%module pywmi

%include "typemaps.i"
%include "scripting/swig/samba.i"

%runtime %{
void push_object(PyObject **stack, PyObject *o)
{
	if ((!*stack) || (*stack == Py_None)) {
    		*stack = o;
	} else {
		PyObject *o2, *o3;
    		if (!PyTuple_Check(*stack)) {
        		o2 = *stack;
        		*stack = PyTuple_New(1);
        		PyTuple_SetItem(*stack,0,o2);
    		}
    		o3 = PyTuple_New(1);
	        PyTuple_SetItem(o3,0,o);
	        o2 = *stack;
    		*stack = PySequence_Concat(o2,o3);
	        Py_DECREF(o2);
    		Py_DECREF(o3);
	}
}
%}

%{
#include "includes.h"
#include "librpc/gen_ndr/misc.h"
#include "librpc/rpc/dcerpc.h"
#include "librpc/rpc/dcerpc_table.h"
#include "libcli/util/nt_status.h"
#include "lib/com/dcom/dcom.h"
#include "librpc/gen_ndr/com_dcom.h"
#include "wmi/wmi.h"
#include "wmi/proto.h"


WERROR WBEM_ConnectServer(struct com_context *ctx, const char *server, const char *nspace, const char *user, const char *password, 
	const char *locale, uint32_t flags, const char *authority, struct IWbemContext* wbem_ctx, struct IWbemServices** services);
WERROR IEnumWbemClassObject_SmartNext(struct IEnumWbemClassObject *d, TALLOC_CTX *mem_ctx, int32_t lTimeout,uint32_t uCount, 
	struct WbemClassObject **apObjects, uint32_t *puReturned);

static PyObject *PyErr_SetFromWERROR(WERROR w);
static PyObject *PyObject_FromCVAR(uint32_t cimtype, union CIMVAR *cvar);
static PyObject *PySWbemObject_FromWbemClassObject(struct WbemClassObject *wco);

static struct com_context *com_ctx;
static PyObject *ComError;
static PyObject *mod_win32_client;
static PyObject *mod_pywintypes;
%}

%apply uint32_t { int32_t };

extern int DEBUGLEVEL;

%wrapper %{
static PyObject *PyErr_SetFromWERROR(WERROR w)
{
	PyObject *v;

	v = Py_BuildValue("(is)", W_ERROR_V(w), wmi_errstr(w));
	if (v != NULL) {
		PyErr_SetObject(ComError, v);
		Py_DECREF(v);
	}
	return NULL;
}

#define RETURN_CVAR_ARRAY(fmt, arr) {\
	PyObject *l, *o;\
	uint32_t i;\
\
	if (!arr) {\
		Py_INCREF(Py_None);\
		return Py_None;\
	}\
	l = PyList_New(arr->count);\
	if (!l) return NULL;\
	for (i = 0; i < arr->count; ++i) {\
		o = _Py_BuildValue(fmt, arr->item[i]);\
		if (!o) {\
			Py_DECREF(l);\
			return NULL;\
		}\
		PyList_SET_ITEM(l, i, o);\
	}\
	return l;\
}

static PyObject *_Py_BuildValue(char *str, ...)
{
   PyObject * result = NULL;
   va_list lst;
   va_start(lst, str);
   if (str && *str == 'I') {
        uint32_t value = va_arg(lst, uint32_t);
	if (value & 0x80000000) {
           result = Py_BuildValue("L", (long)value);
        } else {
           result = Py_BuildValue("i", value);
	}
   } else {
       result = Py_VaBuildValue(str, lst);
   }
   va_end(lst);
   return result;
}

static PyObject *PyObject_FromCVAR(uint32_t cimtype, union CIMVAR *cvar)
{
	switch (cimtype) {
        case CIM_SINT8: return Py_BuildValue("b", cvar->v_sint8);
        case CIM_UINT8: return Py_BuildValue("B", cvar->v_uint8);
        case CIM_SINT16: return Py_BuildValue("h", cvar->v_sint16);
        case CIM_UINT16: return Py_BuildValue("H", cvar->v_uint16);
        case CIM_SINT32: return Py_BuildValue("i", cvar->v_sint32);
        case CIM_UINT32: return _Py_BuildValue("I", cvar->v_uint32);
        case CIM_SINT64: return Py_BuildValue("L", cvar->v_sint64);
        case CIM_UINT64: return Py_BuildValue("K", cvar->v_uint64);
        case CIM_REAL32: return Py_BuildValue("f", cvar->v_real32);
        case CIM_REAL64: return Py_BuildValue("d", cvar->v_real64);
        case CIM_BOOLEAN: return Py_BuildValue("h", cvar->v_boolean);
        case CIM_STRING: return Py_BuildValue("s", cvar->v_string);
        case CIM_DATETIME: return Py_BuildValue("s", cvar->v_datetime);
        case CIM_REFERENCE: return Py_BuildValue("s", cvar->v_reference);
        case CIM_OBJECT: return PySWbemObject_FromWbemClassObject(cvar->v_object);
        case CIM_ARR_SINT8: RETURN_CVAR_ARRAY("b", cvar->a_sint8);
        case CIM_ARR_UINT8: RETURN_CVAR_ARRAY("B", cvar->a_uint8);
        case CIM_ARR_SINT16: RETURN_CVAR_ARRAY("h", cvar->a_sint16);
        case CIM_ARR_UINT16: RETURN_CVAR_ARRAY("H", cvar->a_uint16);
        case CIM_ARR_SINT32: RETURN_CVAR_ARRAY("i", cvar->a_sint32);
        case CIM_ARR_UINT32: RETURN_CVAR_ARRAY("I", cvar->a_uint32);
        case CIM_ARR_SINT64: RETURN_CVAR_ARRAY("L", cvar->a_sint64);
        case CIM_ARR_UINT64: RETURN_CVAR_ARRAY("K", cvar->a_uint64);
        case CIM_ARR_REAL32: RETURN_CVAR_ARRAY("f", cvar->a_real32);
        case CIM_ARR_REAL64: RETURN_CVAR_ARRAY("d", cvar->a_real64);
        case CIM_ARR_BOOLEAN: RETURN_CVAR_ARRAY("h", cvar->a_boolean);
        case CIM_ARR_STRING: RETURN_CVAR_ARRAY("s", cvar->a_string);
        case CIM_ARR_DATETIME: RETURN_CVAR_ARRAY("s", cvar->a_datetime);
        case CIM_ARR_REFERENCE: RETURN_CVAR_ARRAY("s", cvar->a_reference);
	default:
		{
		char *str;
		str = talloc_asprintf(NULL, "Unsupported CIMTYPE(0x%04X)", cimtype);
		PyErr_SetString(PyExc_RuntimeError, str);
		talloc_free(str);
		return NULL;
		}
	}
}

#undef RETURN_CVAR_ARRAY

PyObject *PySWbemObject_InitProperites(PyObject *o, struct WbemClassObject *wco)
{
	PyObject *properties;
	PyObject *addProp;
	uint32_t i;
	int32_t r;
	PyObject *result;

	result = NULL;
	properties = PyObject_GetAttrString(o, "Properties_");
	if (!properties) return NULL;
	addProp = PyObject_GetAttrString(properties, "Add");
	if (!addProp) {
		Py_DECREF(properties);
		return NULL;
	}

	for (i = 0; i < wco->obj_class->__PROPERTY_COUNT; ++i) {
		PyObject *args, *property;

		args = Py_BuildValue("(si)", wco->obj_class->properties[i].name, wco->obj_class->properties[i].desc->cimtype & CIM_TYPEMASK);
		if (!args) goto finish;
		property = PyObject_CallObject(addProp, args);
		Py_DECREF(args);
		if (!property) goto finish;
		if (wco->flags & WCF_INSTANCE) {
			PyObject *value;

			if (wco->instance->default_flags[i] & 1) {
				value = Py_None;
				Py_INCREF(Py_None);
			} else
				value = PyObject_FromCVAR(wco->obj_class->properties[i].desc->cimtype & CIM_TYPEMASK, &wco->instance->data[i]);
			if (!value) {
				Py_DECREF(property);
				goto finish;
			}
			r = PyObject_SetAttrString(property, "Value", value);
			Py_DECREF(value);
			if (r == -1) {
				PyErr_SetString(PyExc_RuntimeError, "Error setting value of property");
				goto finish;
			}
		}
		Py_DECREF(property);
	}

	Py_INCREF(Py_None);
	result = Py_None;
finish:
	Py_DECREF(addProp);
	Py_DECREF(properties);
	return result;
}

static PyObject *PySWbemObject_FromWbemClassObject(struct WbemClassObject *wco)
{
	PyObject *swo_class, *swo, *args, *result;

	swo_class = PyObject_GetAttrString(mod_win32_client, "SWbemObject");
	if (!swo_class) return NULL;
	args = PyTuple_New(0);
	if (!args) {
		Py_DECREF(swo_class);
		return NULL;
	}
	swo = PyObject_CallObject(swo_class, args);
	Py_DECREF(args);
	Py_DECREF(swo_class);
	if (!swo) return NULL;

	result = PySWbemObject_InitProperites(swo, wco);
	if (!result) {
		Py_DECREF(swo);
		return NULL;
	}
	Py_DECREF(result);

	return swo;
}

%}

%typemap(out) WERROR {
	if (!W_ERROR_IS_OK($1)) {
		PyErr_SetFromWERROR($1);
		return NULL;
	}
        $result = Py_None;
        Py_INCREF(Py_None);
}

%typemap(in, numinputs=0) struct com_context *ctx {
	$1 = com_ctx;
}

%typemap(in, numinputs=0) TALLOC_CTX *mem_ctx {
	$1 = NULL;
}

%typemap(in, numinputs=0) struct IWbemServices **services (struct IWbemServices *temp) {
	$1 = &temp;
}

%typemap(argout) struct IWbemServices **services {
	PyObject *o;
	o = SWIG_NewPointerObj(*$1, SWIGTYPE_p_IWbemServices, 0);
	push_object(&$result, o);
}

WERROR WBEM_ConnectServer(struct com_context *ctx, const char *server, const char *nspace, const char *user, const char *password,
        const char *locale, uint32_t flags, const char *authority, struct IWbemContext* wbem_ctx, struct IWbemServices** services);

%typemap(in, numinputs=0) struct IEnumWbemClassObject **ppEnum (struct IEnumWbemClassObject *temp) {
	$1 = &temp;
}

%typemap(argout) struct IEnumWbemClassObject **ppEnum {
	PyObject *o;
	o = SWIG_NewPointerObj(*$1, SWIGTYPE_p_IEnumWbemClassObject, 0);
	push_object(&$result, o);
}


uint32_t IUnknown_Release(void *d, TALLOC_CTX *mem_ctx);

WERROR IWbemServices_ExecQuery(struct IWbemServices *d, TALLOC_CTX *mem_ctx, const char *strQueryLanguage, const char *strQuery, 
	int32_t lFlags, struct IWbemContext *pCtx, struct IEnumWbemClassObject **ppEnum);

WERROR IWbemServices_ExecNotificationQuery(struct IWbemServices *d, TALLOC_CTX *mem_ctx, const char *strQueryLanguage, const char *strQuery, 
	int32_t lFlags, struct IWbemContext *pCtx, struct IEnumWbemClassObject **ppEnum);

WERROR IWbemServices_CreateInstanceEnum(struct IWbemServices *d, TALLOC_CTX *mem_ctx, const char *strClass, 
	int32_t lFlags, struct IWbemContext *pCtx, struct IEnumWbemClassObject **ppEnum);

WERROR IEnumWbemClassObject_Reset(struct IEnumWbemClassObject *d, TALLOC_CTX *mem_ctx);

%typemap(in, numinputs=1) (uint32_t uCount, struct WbemClassObject **apObjects, uint32_t *puReturned) (uint32_t uReturned) {
        if (PyLong_Check($input))
    		$1 = PyLong_AsUnsignedLong($input);
        else if (PyInt_Check($input))
    		$1 = PyInt_AsLong($input);
        else {
            PyErr_SetString(PyExc_TypeError,"Expected a long or an int");
            return NULL;
        }
	$2 = talloc_array(NULL, struct WbemClassObject *, $1);
	$3 = &uReturned;
}

%typemap(argout) (struct WbemClassObject **apObjects, uint32_t *puReturned) {
	uint32_t i;
	PyObject *o;
	int32_t error;

	error = 0;

	$result = PyTuple_New(*$2);
	for (i = 0; i < *$2; ++i) {
		if (!error) {
			o = PySWbemObject_FromWbemClassObject($1[i]);
			if (!o)
				--error;
			else
			    error = PyTuple_SetItem($result, i, o);
		}
		talloc_free($1[i]);
	}
	talloc_free($1);
	if (error) return NULL;
}

%typemap(out) WERROR {
	if (!W_ERROR_IS_OK($1)) {
		PyErr_SetFromWERROR($1);
		talloc_free(arg5); // FIXME:avg make it properly(how???)
		return NULL;
	}
}

WERROR IEnumWbemClassObject_SmartNext(struct IEnumWbemClassObject *d, TALLOC_CTX *mem_ctx, int32_t lTimeout, uint32_t uCount, 
	struct WbemClassObject **apObjects, uint32_t *puReturned);

%init %{

	mod_win32_client = PyImport_ImportModule("win32com.client");
	mod_pywintypes = PyImport_ImportModule("pywintypes");
	ComError = PyObject_GetAttrString(mod_pywintypes, "com_error");

	DEBUGLEVEL = 0;
//	talloc_enable_leak_report_full();

	lp_load();
        dcerpc_init();
        dcerpc_table_init();

        dcom_proxy_IUnknown_init();
        dcom_proxy_IWbemLevel1Login_init();
        dcom_proxy_IWbemServices_init();
        dcom_proxy_IEnumWbemClassObject_init();
        dcom_proxy_IRemUnknown_init();
        dcom_proxy_IWbemFetchSmartEnum_init();
        dcom_proxy_IWbemWCOSmartEnum_init();
        dcom_proxy_IWbemClassObject_init();

        com_init_ctx(&com_ctx, NULL);
        dcom_client_init(com_ctx, NULL);

    {
	PyObject *pModule;

	pModule = PyImport_ImportModule( "win32com.client" );
    }
%}

// ************ winreg **************

%{
#include "auth/credentials/credentials.h"
#include "librpc/gen_ndr/ndr_winreg_c.h"
#include "librpc/gen_ndr/winreg.h"
static PyObject *_Py_BuildValue(char *str, ...);
%}

#define REG_NONE 0
#define REG_SZ 1
#define REG_EXPAND_SZ 2
#define REG_BINARY 3
#define REG_DWORD_LITTLE_ENDIAN 4
#define REG_DWORD 4
#define REG_DWORD_BIG_ENDIAN 5
#define REG_LINK 6
#define REG_MULTI_SZ 7
#define REG_RESOURCE_LIST 8
#define REG_FULL_RESOURCE_DESCRIPTOR 9
#define REG_RESOURCE_REQUIREMENTS_LIST 10
#define REG_QWORD_LITTLE_ENDIAN 11
#define REG_QWORD 11

%typemap(in,numinputs=0) (struct policy_handle *out_handle) {
	$1 = talloc(0, struct policy_handle);
}

%typemap(argout) (struct policy_handle *out_handle) {
	PyObject *o;
	o = SWIG_NewPointerObj($1, SWIGTYPE_p_policy_handle, 0);
	push_object(&$result, o);
}

%typemap(in) struct winreg_String {
	$1.name = PyString_AsString($input);
	$1.name_len = 2*PyString_Size($input);
	$1.name_size = $1.name_len;
}

%typemap(in,numinputs=0) (WERROR *errcode) (WERROR temp) {
	$1 = &temp;
}

%typemap(argout) (WERROR *errcode) {
        if (!W_ERROR_IS_OK(*$1)) {
                PyErr_SetFromWERROR(*$1);
                return NULL;
	}
}

%inline {
struct winreg { struct dcerpc_pipe *pipe; };
}

%extend winreg {
        winreg (const char *binding, WERROR *errcode, struct cli_credentials *cred = NULL, TALLOC_CTX *mem_ctx = NULL, struct event_context *event = NULL)
        {
                struct winreg *ret = talloc(mem_ctx, struct winreg);
                NTSTATUS status;

                status = dcerpc_pipe_connect(mem_ctx, &ret->pipe, binding, &dcerpc_table_winreg, cred, event);
                *errcode = ntstatus_to_werror(status);
                if (NT_STATUS_IS_ERR(status)) {
            		talloc_free(ret);
                        return NULL;
                }
                return ret;
        }

        ~winreg() {
                talloc_free(self);
        }

%typemap(out) WERROR {
	if (!W_ERROR_IS_OK($1)) {
		PyErr_SetFromWERROR($1);
		talloc_free(arg3); // FIXME:avg make it properly(how???)
		return NULL;
	}
}

        WERROR OpenHKLM(uint32_t access_mask, struct policy_handle *out_handle)
        {
                struct winreg_OpenHKLM r;
                NTSTATUS status;

                r.in.system_name = NULL;
                r.in.access_mask = access_mask;
		r.out.handle = out_handle;

                status = dcerpc_winreg_OpenHKLM(self->pipe, NULL, &r);
                if (NT_STATUS_IS_ERR(status)) {
                        return ntstatus_to_werror(status);
                }

                return r.out.result;
        }

        WERROR OpenHKPT(uint32_t access_mask, struct policy_handle *out_handle)
        {
                struct winreg_OpenHKPT r;
                NTSTATUS status;

                r.in.system_name = NULL;
                r.in.access_mask = access_mask;
		r.out.handle = out_handle;

                status = dcerpc_winreg_OpenHKPT(self->pipe, NULL, &r);
                if (NT_STATUS_IS_ERR(status)) {
                        return ntstatus_to_werror(status);
                }
                return r.out.result;
        }

%typemap(out) WERROR {
	if (!W_ERROR_IS_OK($1)) {
		PyErr_SetFromWERROR($1);
		return NULL;
	} else {
		Py_INCREF(Py_None);
		$result = Py_None;
	}
}

        WERROR CloseKey(struct policy_handle *handle)
        {
                struct winreg_CloseKey r;
                NTSTATUS status;

                r.in.handle = handle;
		r.out.handle = handle;

                status = dcerpc_winreg_CloseKey(self->pipe, NULL, &r);
                if (NT_STATUS_IS_ERR(status)) {
                        return ntstatus_to_werror(status);
                }

                return r.out.result;
        }

        PyObject *QueryValue(struct policy_handle *handle, const char *name, uint32_t size)
        {
                struct winreg_QueryValue r;
                NTSTATUS status;
		enum winreg_Type type;
		void *loc_ctx;
		PyObject *resultobj;
		uint32_t length;

                r.in.handle = handle;
                r.in.value_name.name = name;
		r.in.value_name.name_len = 2 * strlen_m_term(name);
		r.in.value_name.name_size = r.in.value_name.name_len;
                r.in.type = &type;
		r.in.size = &size;
		r.in.length = &length;
		r.out.type = &type;
		r.out.size = &size;
		r.out.length = &length;
		length = 0;
		loc_ctx = talloc_new(NULL);
		if (!size) {
		    r.in.data = NULL;
            	    status = dcerpc_winreg_QueryValue(self->pipe, loc_ctx, &r);
            	    if (NT_STATUS_IS_ERR(status)) {
			PyErr_SetFromWERROR(ntstatus_to_werror(status));
			talloc_free(loc_ctx);
                        return NULL;
            	    }
		    //if (W_ERROR_EQUAL(r.out.result, WERR_MORE_DATA));
    		    if (!W_ERROR_IS_OK(r.out.result)) {
            		PyErr_SetFromWERROR(r.out.result);
			talloc_free(loc_ctx);
            		return NULL;
		    }
		    size = *r.out.size;
		}

		r.out.data = r.in.data = talloc_array(loc_ctx, uint8_t, *r.out.size);
		status = dcerpc_winreg_QueryValue(self->pipe, loc_ctx, &r);
                if (NT_STATUS_IS_ERR(status)) {
                        PyErr_SetFromWERROR(ntstatus_to_werror(status));
                        talloc_free(loc_ctx);
                        return NULL;
                }
    		if (!W_ERROR_IS_OK(r.out.result)) {
            		PyErr_SetFromWERROR(r.out.result);
			talloc_free(loc_ctx);
            		return NULL;
		}

		resultobj = PyInt_FromLong(*r.out.type);
		switch (*r.out.type) {
		case REG_MULTI_SZ: {
			const char *s;
			int rlen = convert_string_talloc(loc_ctx, CH_UTF16, CH_UNIX, r.out.data, *r.out.size, (void **)&s);
			if (rlen < 0) {
			    PyErr_SetString(PyExc_RuntimeError, "REG_MULTI_SZ char conversion problem");
			    Py_DECREF(resultobj);
			    talloc_free(loc_ctx);
            		    return NULL;
			}
			while (*s && rlen > 0) {
			    int l = strnlen(s, rlen);
			    push_object(&resultobj, PyString_FromStringAndSize(s, l));
			    if (l < rlen) ++l;
			    s += l;
			    rlen -= l;
			}
			break;
		}
		case REG_DWORD:
			push_object(&resultobj, _Py_BuildValue("I", *((uint32_t *)r.out.data)));
			break;
		case REG_BINARY:
			push_object(&resultobj, PyString_FromStringAndSize((const char *)r.out.data, *r.out.size));
			break;
		default:
			DEBUG(0,(__location__ ": winreg type: %d\n", *r.out.type));
			PyErr_SetString(PyExc_RuntimeError, "QueryValue: Unsupported out winreg type");
			return NULL;
		}

		talloc_free(loc_ctx);
                return resultobj;
        }

}
