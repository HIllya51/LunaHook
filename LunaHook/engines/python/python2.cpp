#include"types.h"
#include"python.h"
#include"main.h"
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

    inline std::pair<Py_UNICODE*,Py_ssize_t> GetPyUnicodeString(PyObject *object){
        if(PyUnicode_FromObject==NULL)
            return {};
        if (object == NULL)
            return {};
        
        auto uformat = PyUnicode_FromObject(object);
        
        if (uformat == NULL){
            return {};
        }
        
        auto fmt = PyUnicode_AS_UNICODE(uformat);
        auto fmtcnt = PyUnicode_GET_SIZE(uformat);
        return {fmt,fmtcnt};   
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
                PyUnicode_FromObject=(PyUnicode_FromObject_t)GetProcAddress(module, "PyUnicodeUCS2_FromObject");
                PyUnicode_FromUnicode=(PyUnicode_FromUnicode_t)GetProcAddress(module, "PyUnicodeUCS2_FromUnicode");
                auto f1=[=](){
                    HookParam hp;
                    hp.address = (uintptr_t)GetProcAddress(module, "PyUnicodeUCS2_Format");
                    if (!hp.address) return false;
                    
                    hp.text_fun = [](hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len)
                    {
                        #ifndef _WIN64
                        auto format=(PyObject *)stack->stack[1];
                        #else
                        auto format=(PyObject *)stack->rcx;
                        #endif
                        auto [strptr,strlen]=GetPyUnicodeString(format);
                        *data=(uintptr_t)strptr;
                        *len=0;
                        
                        if(wcschr(strptr, L'%') == nullptr)
                            *len=sizeof(wchar_t)*strlen;
                        
                    };
                    hp.type = USING_STRING | CODEC_UTF16 | NO_CONTEXT;
                    if(PyUnicode_FromUnicode)
                    {
                        hp.type|=EMBED_ABLE|EMBED_BEFORE_SIMPLE;
                        hp.hook_after=[](hook_stack* stack,void* data, size_t len)
                            {
                                #ifndef _WIN64
                                auto format=(PyObject *)stack->stack[1];
                                #else
                                auto format=(PyObject *)stack->rcx;
                                #endif
                                if(format==NULL)return;
                                #ifndef _WIN64
                                stack->stack[1]=
                                #else
                                stack->rcx=
                                #endif
                                    (uintptr_t)PyUnicode_FromUnicode((Py_UNICODE *)data,len/2);
                            };
                        hookrenpy(module);
                    }
                    return NewHook(hp, "Ren'py");
                }();
                
                #if 0
                auto f2=[=](){
                    HookParam hp;
                    hp.address = (uintptr_t)GetProcAddress(module, "PyUnicodeUCS2_Concat");
                    if (!hp.address) return false; 
                    hp.text_fun = [](hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len)
                    {
                        auto left=(PyObject *)stack->stack[1]; 
                        auto right=(PyObject *)stack->stack[2]; 
                         
                        auto [strptr,strlen]=GetPyUnicodeString(right);
                        *data=(uintptr_t)strptr;
                        *len=sizeof(wchar_t)*strlen;
                    }; 
                    hp.filter_fun = [](void* data, size_t* len, HookParam* hp)
                    {
                        auto str=std::wstring(reinterpret_cast<LPWSTR>(data),*len/2);
                        auto filterpath={
                            L".rpy",L".rpa",L".py",L".pyc",L".txt",
                            L".png",L".jpg",L".bmp",
                            L".mp3",L".ogg",
                            L".webm",L".mp4",
                            L".otf",L".ttf"
                        };
                        for(auto _ :filterpath)
                            if(str.find(_)!=str.npos)
                                return false;
                        return true;
                    };
                    hp.type = USING_STRING | CODEC_UTF16;
                    //hp.filter_fun = [](void* str, auto, auto, auto) { return *(wchar_t*)str != L'%'; };
                    return NewHook(hp, "Ren'py");
                }();
                #else
                auto f2=false;
                #endif
                return f1||f2;
            }
        }
    }
	ConsoleOutput("Ren'py failed: failed to find python2X.dll");
	return false;
}
 