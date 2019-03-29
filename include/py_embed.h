#ifndef PY_EMBED
#define PY_EMBED

#include <stdbool.h>
#include <wchar.h>

#include "python3.7m/Python.h"

#define MODULE_FUNCTION_LOAD_FAILURE -1

typedef PyObject* py_object;

typedef size_t py_function;

typedef struct {
  wchar_t* program_name;
  size_t function_count;
  py_object module;
  py_object* module_functions;
} py_embed;

#define destroy_py_object(obj) Py_DECREF(obj);

#define call_py_func_args(embed, func, argc, ...)                            \
  ({                                                                         \
    py_object args[] = {__VA_ARGS__};                                        \
    py_object py_args = PyTuple_New(argc);                                   \
    for (int i = 0; i < argc; ++i) {                                         \
      PyTuple_SetItem(py_args, i, args[i]);                                  \
    }                                                                        \
    py_object result =                                                       \
        PyObject_CallObject(embed->module_functions[(size_t)func], py_args); \
    destroy_py_object(py_args);                                              \
    result;                                                                  \
  })

#define call_py_func(embed, func) \
  ({ PyObject_CallObject(embed->module_functions[(size_t)func], NULL); })

#define call_py_func_args_void(embed, func, argc, ...)                      \
  ({                                                                        \
    py_object result = call_py_func_args(embed, func, argc, ##__VA_ARGS__); \
    if (result) destroy_py_object(result);                                  \
  })

#define call_py_func_void(embed, func)            \
  ({                                              \
    py_object result = call_py_func(embed, func); \
    if (result) destroy_py_object(result);        \
  })

// initializes the Python interpreter
py_embed* create_py_embed(const char* argv[]);

// cleans up resources allocated by create_py_embed()
// and de-initializes the Python interpreter
//
// returns 0 on successful de-initialization of the
// Python interpreter
int destroy_py_embed(py_embed* embed);

// loads a python module from disk into the embedded interpreter,
// overwriting the previously-loaded module if one exists
int load_py_module(py_embed* const embed, const char* const path);

// loads a function by name from the embedded interpreter's loaded
// module and returns a value that can be used to call it later
py_function load_py_function(py_embed* const embed, const char* const name);

py_object create_py_long(long value);

py_object create_py_float(double value);

py_object create_py_bool(bool value);

#endif