#pragma once
#include <tuple>

#include <boost/lexical_cast.hpp>


struct PyObject
{
    int refc;
    void* inf;
};

#define Py_single_input 256
#define Py_file_input 257
#define Py_eval_input 258

#define SQUARE(x) ((x) * (x))

template<class T1, class T2>
inline double getDistanceSqr(const T1& a, const T2& b)
{
    return SQUARE((double)a.x - b.x) + SQUARE((double)a.y - b.y);
}

template<class T1, class T2>
inline auto getDistanceSqrAuto(const T1& a, const T2& b)
{
    return SQUARE(a.x - b.x) + SQUARE(a.y - b.y);
}

typedef PyObject* (*PythonFunctionEmpty)();
typedef PyObject* (*PythonFunctionSzIPoPo)(const char*, int, PyObject*, PyObject*);
typedef PyObject* (*PythonFunctionPo)(PyObject*);

extern void(__stdcall* chat_append_og)(int, const char*);
extern void(__stdcall* whisper_append_og)(int, const char*, const char*);

extern PythonFunctionEmpty PyEval_GetGlobals;
extern PythonFunctionEmpty PyEval_GetLocals;
extern PythonFunctionSzIPoPo PyRun_String;
extern PythonFunctionPo PyObject_Repr;

extern const char* (*PyString_AsString)(PyObject*);

extern int (*PyRun_SimpleString)(const char*);

template<typename... Targs>
inline PyObject* EvalPy(const char* format, Targs... args)
{
    char buff[1024];
    sprintf_s(buff, format, args...);

    PyObject* result = PyRun_String(buff, Py_eval_input, PyEval_GetGlobals(), PyEval_GetLocals());
    PyRun_SimpleString("\n");

    return result;
}

template<typename... Targs>
inline const char* EvalPyGetRepr(const char* eval, Targs... args)
{
    return PyString_AsString(PyObject_Repr(EvalPy(eval, args...)));
}

