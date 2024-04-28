#include "XENOX_functions.h"


void(__stdcall* chat_append_og)(int, const char*) = (decltype(chat_append_og))0x00'4a'9f'00;
void(__stdcall* whisper_append_og)(int, const char*, const char*) = (decltype(whisper_append_og))0x00'4a'a2'60;

PythonFunctionEmpty PyEval_GetGlobals = (PythonFunctionEmpty)0x1e'0f'1f'f0;
PythonFunctionEmpty PyEval_GetLocals = (PythonFunctionEmpty)0x1e'0f'20'00;
PythonFunctionSzIPoPo PyRun_String = (PythonFunctionSzIPoPo)0x1e'11'af'd0;
PythonFunctionPo PyObject_Repr = (PythonFunctionPo)0x1e'0a'fe'50;

const char* (*PyString_AsString)(PyObject*) = (decltype(PyString_AsString))0x1e'0b'b9'60;

int (*PyRun_SimpleString)(const char*) = (decltype(PyRun_SimpleString))0x1e'11'af'40;
