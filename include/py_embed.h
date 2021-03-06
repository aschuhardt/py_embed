#ifndef PY_EMBED
#define PY_EMBED

#include <stdbool.h>
#include <wchar.h>

#define MODULE_FUNCTION_LOAD_FAILURE -1

#define py_decomp(type, obj) decompose_py_##type(obj)
#define py_check(type, obj) is_py_##type(obj)
#define py_obj(type, obj) create_py_##type(obj)

typedef void* py_object;

typedef size_t py_function;

typedef struct {
  char* mod_name;
  size_t func_count;
  size_t func_index;
  void* functions;
} module_injection;

typedef struct {
  wchar_t* program_name;
  size_t function_count;
  py_object module;
  py_object* module_functions;
  module_injection inject_state;
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

// creates a Python integer object from a signed long integer
py_object create_py_long(long value);

// consumes a Python integer object, returning the value as a signed long
// integer and deallocating the Python object
long decompose_py_long(py_object obj);

// returns true if the provided object is a Python integer object
bool is_py_long(py_object obj);

// creates a Python floating-point object from a double-precision floating-point
// value
py_object create_py_float(double value);

// consumes a Python floating-point object, returning the value as a
// double-precision floating-point value and deallocating the Python object
double decompose_py_float(py_object obj);

// returns true if the provided object is a Python floating-point object
bool is_py_float(py_object obj);

// creates a Python Boolean object from a bool value
py_object create_py_bool(bool value);

// consumes a Python Boolean object, returning the value as a bool and
// deallocating the Python object
bool decompose_py_bool(py_object obj);

// returns true if the provided object is a Python Boolean object
bool is_py_bool(py_object obj);

// creates a Python string object from a string
py_object create_py_str(const char* const contents);

// consumes a Python string object, returning the value as a string and
// deallocating the Python object
char* decompose_py_str(py_object obj);

// returns true if the provided object is a Python string object
bool is_py_str(py_object obj);

void py_begin_module_injection(py_embed* const embed, size_t function_count,
                               const char* const name);

void py_add_injected_function(py_embed* const embed, const char* const name,
                              const char* const desc,
                              py_object (*func)(py_object, py_object));

void py_finish_module_injection(py_embed* const embed);

#endif