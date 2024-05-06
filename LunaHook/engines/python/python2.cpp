#include"python.h"
namespace {
    typedef wchar_t Py_UNICODE ;
    typedef size_t         Py_ssize_t;
    typedef void PyObject ;
    typedef  PyObject* (*PyUnicode_FromObject_t)(  PyObject *obj  );
    #ifdef Py_TRACE_REFS
    /* Define pointers to support a doubly-linked list of all live heap objects. */
    #define _PyObject_HEAD_EXTRA            \
        struct _object *_ob_next;           \
        struct _object *_ob_prev;

    #define _PyObject_EXTRA_INIT 0, 0,

    #else
    #define _PyObject_HEAD_EXTRA
    #define _PyObject_EXTRA_INIT
    #endif
    #define PyObject_HEAD                   \
        _PyObject_HEAD_EXTRA                \
        Py_ssize_t ob_refcnt;               \
        struct _typeobject *ob_type;
    typedef struct {
        PyObject_HEAD
        Py_ssize_t length;          /* Length of raw Unicode data in buffer */
        Py_UNICODE *str;            /* Raw Unicode buffer */
        long hash;                  /* Hash value; -1 if not set */
        PyObject *defenc;           /* (Default) Encoded version as Python
                                    string, or NULL; this is used for
                                    implementing the buffer protocol */
    } PyUnicodeObject;
    #define PyUnicode_AS_UNICODE(op) \
        (((PyUnicodeObject *)(op))->str)
    #define PyUnicode_GET_SIZE(op) \
        (((PyUnicodeObject *)(op))->length)

    PyUnicode_FromObject_t PyUnicode_FromObject;

    inline void GetPyUnicodeString(PyObject *object,uintptr_t* data, size_t* len){
        if (object == NULL)
            return;
        
        auto uformat = PyUnicode_FromObject(object);
        
        if (uformat == NULL){
            return;
        }
        
        auto fmt = PyUnicode_AS_UNICODE(uformat);
        auto fmtcnt = PyUnicode_GET_SIZE(uformat);
        *data=(uintptr_t)fmt;
        
        if(wcschr(fmt, L'%') == nullptr)
            *len=sizeof(wchar_t)*fmtcnt;
    }

    typedef PyObject* (*PyUnicode_FromUnicode_t)(
        const Py_UNICODE *u,        /* Unicode buffer */
        Py_ssize_t size             /* size of buffer */
        );
    PyUnicode_FromUnicode_t PyUnicode_FromUnicode;
    
}
 
bool InsertRenpyHook(){
    wchar_t python[] = L"python2X.dll", libpython[] = L"libpython2.X.dll";
    for (wchar_t* name : { python, libpython })
    {
        wchar_t* pos = wcschr(name, L'X');
        for (int pythonMinorVersion = 0; pythonMinorVersion <= 8; ++pythonMinorVersion)
        {
            *pos = L'0' + pythonMinorVersion;
            if (HMODULE module = GetModuleHandleW(name))
            {
                auto f1=[=](){
                    PyUnicode_FromObject=(PyUnicode_FromObject_t)GetProcAddress(module, "PyUnicodeUCS2_FromObject");
                    PyUnicode_FromUnicode=(PyUnicode_FromUnicode_t)GetProcAddress(module, "PyUnicodeUCS2_FromUnicode");
                    auto addr= (uintptr_t)GetProcAddress(module, "PyUnicodeUCS2_Format");
                    if (!addr||!PyUnicode_FromObject) return false;
                    HookParam hp;
                    hp.address =addr;
                    hp.type = USING_STRING | CODEC_UTF16 | NO_CONTEXT;
                    hp.text_fun = [](hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len)
                    {
                        auto format=(PyObject *)stack->ARG1;
                        GetPyUnicodeString(format,data,len);
                    };
                    if(PyUnicode_FromUnicode)
                    {
                        hp.type|=EMBED_ABLE|EMBED_BEFORE_SIMPLE;
                        hp.hook_after=[](hook_stack* stack,void* data, size_t len)
                            {
                                auto format=(PyObject *)stack->ARG1;
                                if(format==NULL)return;
                                stack->ARG1=(uintptr_t)PyUnicode_FromUnicode((Py_UNICODE *)data,len/2);
                            };
                    }
                    return NewHook(hp, "Ren'py");
                }();
                auto f3=hookrenpy(module);
                
                return f1||f3;
            }
        }
    }
	ConsoleOutput("Ren'py failed: failed to find python2X.dll");
	return false;
}
 