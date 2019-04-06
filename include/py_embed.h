#ifndef PY_EMBED
#define PY_EMBED

#include <stdbool.h>
#include <wchar.h>

#define MODULE_FUNCTION_LOAD_FAILURE -1

typedef void* py_object;

typedef size_t py_function;

typedef struct {
  wchar_t* program_name;
  size_t function_count;
  py_object module;
  py_object* module_functions;
} py_embed;

// disposes of a Python object
void destroy_py_object(py_object obj);

// calls a registered Python function, passing it the provided arguments
// and returning the result of the function call
py_object call_py_func_args(py_embed* const embed, py_function func,
                            unsigned int argc, ...);

// calls a registered Python function, returning the result of the
// function call
py_object call_py_func(py_embed* const embed, py_function func);

// calls a registered Python function, passing it the provided arguments
// and disposing of any result
void call_py_func_args_void(py_embed* const embed, py_function func,
                            unsigned int argc, ...);

// calls a registered Python function, disposing of any result
void call_py_func_void(py_embed* const embed, py_function func);

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