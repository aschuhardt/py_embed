#include <float.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Python.h"

#include "py_embed.h"

#define PY_EMBED_FAILURE 0
#define PY_EMBED_SUCCESS 1

#define PRINT_PY_ERROR() \
  if (PyErr_Occurred()) PyErr_Print();

#define PY_NULL_ERR_CHECK(ptr) \
  if (!ptr) {                  \
    PRINT_PY_ERROR();          \
    return PY_EMBED_FAILURE;   \
  }

static inline void destroy_object(PyObject* obj) {
  if (obj) Py_DECREF(obj);
}

void destroy_py_object(py_object obj) { destroy_object((PyObject*)obj); }

py_object call_py_func_variadic(py_embed* const embed, py_function func,
                                unsigned int argc, va_list args) {
  PyObject* py_args = PyTuple_New(argc);

  for (int i = 0; i < argc; ++i) {
    PyTuple_SetItem(py_args, i, va_arg(args, PyObject*));
  }

  return PyObject_CallObject(embed->module_functions[(size_t)func], py_args);
}

py_object call_py_func(py_embed* const embed, py_function func) {
  PyObject* result =
      PyObject_CallObject(embed->module_functions[(size_t)func], NULL);

  PRINT_PY_ERROR();

  return (py_object)result;
}

py_object call_py_func_args(py_embed* const embed, py_function func,
                            unsigned int argc, ...) {
  PyObject* result;

  if (argc > 0) {
    va_list args;
    va_start(args, argc);
    result = call_py_func_variadic(embed, func, argc, args);
    va_end(args);
  } else {
    result = call_py_func(embed, func);
  }

  PRINT_PY_ERROR();

  return (py_object)result;
}

void call_py_func_args_void(py_embed* const embed, py_function func,
                            unsigned int argc, ...) {
  PyObject* result;

  if (argc > 0) {
    va_list args;
    va_start(args, argc);
    result = call_py_func_variadic(embed, func, argc, args);
    va_end(args);
  } else {
    result = call_py_func(embed, func);
  }

  if (result) Py_DECREF(result);
}

void call_py_func_void(py_embed* const embed, py_function func) {
  PyObject* result = call_py_func(embed, func);
  if (result) Py_DECREF(result);
}

static void cleanup_injection_artifacts(py_embed* const embed) {
  // cleanup
  if (embed->inject_mod_name) free(embed->inject_mod_name);
  if (embed->inject_funcs) {
    for (int i = 0; i < embed->inject_func_count; ++i) {
      PyMethodDef* def = &((PyMethodDef*)(embed->inject_funcs))[i];
      if (def) {
        if (def->ml_name) free((char*)def->ml_name);
        if (def->ml_doc) free((char*)def->ml_doc);
      }
    }
    free(embed->inject_funcs);
  }
}

py_embed* create_py_embed(const char* argv[]) {
  py_embed* embed = malloc(sizeof(py_embed));

  embed->module_functions = NULL;

  // set the program name from the name of the executing binary
  embed->program_name = Py_DecodeLocale(argv[0], NULL);
  Py_SetProgramName(embed->program_name);

  return embed;
}

int destroy_py_embed(py_embed* embed) {
  if (embed->module_functions) {
    for (int i = 0; i < embed->function_count; ++i) {
      py_object func = embed->module_functions[i];
      if (func) destroy_py_object(func);
    }

    free(embed->module_functions);
  }

  // Py_FinalizeEx() should indicate whether the python interpreter
  // was deinitialized successfully
  int error_code = Py_FinalizeEx() < 0 ? 120 : 0;

  PRINT_PY_ERROR();

  // clean up embed state resources
  PyMem_RawFree(embed->program_name);
  destroy_py_object(embed->module);

  cleanup_injection_artifacts(embed);

  free(embed);

  return error_code;
}

int load_py_module(py_embed* const embed, const char* const path) {
  // if an existing module is already loaded, dispose of it
  destroy_py_object(embed->module);

  Py_Initialize();

  PyObject* module = PyModule_New("embed");
  PY_NULL_ERR_CHECK(module);

  embed->module = (py_object)module;

  PyObject* locals = PyModule_GetDict(module);
  PyObject* builtins = PyEval_GetBuiltins();

  PyModule_AddStringConstant(module, "__file__", path);
  PyDict_SetItemString(locals, "__builtins__", builtins);

  FILE* fp = fopen(path, "r");
  if (!fp) {
    perror("Failed to open file");
    return PY_EMBED_FAILURE;
  }

  // file handle is closed when this function finishes
  PyObject* script = PyRun_FileEx(fp, path, Py_file_input, locals, locals, 1);

  PY_NULL_ERR_CHECK(script);

  return PY_EMBED_SUCCESS;
}

py_function load_py_function(py_embed* const embed, const char* const name) {
  if (!embed->module || !embed || !name) {
    PRINT_PY_ERROR();
    return MODULE_FUNCTION_LOAD_FAILURE;
  }

  if (embed->function_count == 0) {
    // if no functions have been added before now, malloc the function pointer
    // buffer accomodate the first one
    embed->module_functions = (py_object*)malloc(sizeof(PyObject*));
  } else {
    // if this is not the first function to be loaded, the expand the function
    // pointer buffer to fit the new size
    embed->module_functions =
        (py_object*)realloc((void*)embed->module_functions,
                            sizeof(py_object) * (embed->function_count + 1));
  }

  PyObject* func = PyObject_GetAttrString(embed->module, name);

  if (func && PyCallable_Check(func)) {
    size_t index = embed->function_count;
    embed->module_functions[index] = (py_object)func;

    ++(embed->function_count);

    return (py_function)index;
  } else {
    PRINT_PY_ERROR();
    return (py_function)MODULE_FUNCTION_LOAD_FAILURE;
  }
}

py_object create_py_long(long value) {
  PyObject* obj = PyLong_FromLong(value);
  if (!obj) PRINT_PY_ERROR();
  return (py_object)obj;
}

long decompose_py_long(py_object obj) {
  PyObject* pyobj = (PyObject*)obj;
  if (!pyobj) return LONG_MIN;
  long value = PyLong_AsLong(pyobj);
  Py_DECREF(pyobj);
  return value;
}

bool is_py_long(py_object obj) {
  return PyLong_Check((PyObject*)obj) ? true : false;
}

py_object create_py_float(double value) {
  PyObject* obj = PyLong_FromDouble(value);
  if (!obj) PRINT_PY_ERROR();
  return (py_object)obj;
}

double decompose_py_float(py_object obj) {
  PyObject* pyobj = (PyObject*)obj;
  if (!pyobj) return DBL_MIN;
  double value = PyFloat_AsDouble(pyobj);
  Py_DECREF(pyobj);
  return value;
}

bool is_py_float(py_object obj) {
  return PyFloat_Check((PyObject*)obj) ? true : false;
}

py_object create_py_bool(bool value) {
  if (value)
    Py_RETURN_TRUE;
  else
    Py_RETURN_FALSE;
}

bool decompose_py_bool(py_object obj) {
  int comp = PyObject_RichCompareBool(obj, Py_True, Py_EQ);
  Py_DECREF(obj);
  if (comp < 0) PRINT_PY_ERROR();
  return comp ? true : false;
}

bool is_py_bool(py_object obj) {
  return PyBool_Check((PyObject*)obj) ? true : false;
}

py_object create_py_str(const char* const contents) {
  PyObject* obj = PyUnicode_FromString(contents);
  if (!obj) PRINT_PY_ERROR();
  return (py_object)obj;
}

char* decompose_py_str(py_object obj) {
  PyObject* pyobj = (PyObject*)obj;
  if (!pyobj || PyUnicode_READY(obj) < 0) {
    PRINT_PY_ERROR();
    return NULL;
  }
  if (PyUnicode_KIND(obj) != PyUnicode_1BYTE_KIND) {
    return "(Error: only UTF-8 encoding is currently supported.)";
  }
  Py_ssize_t length = PyUnicode_GET_LENGTH(obj);
  return (char*)PyUnicode_1BYTE_DATA(obj);
}

bool is_py_str(py_object obj) { return PyUnicode_Check((PyObject*)obj); }

void py_begin_module_injection(py_embed* const embed, size_t function_count,
                               const char* const name) {
  // make sure no artifacts from a previous injection are leaked
  cleanup_injection_artifacts(embed);

  // copy the name of the module to the embed's inject_mod_name
  // field
  size_t name_length = strlen(name);
  embed->inject_mod_name = (char*)malloc(sizeof(char) * name_length);
  strncpy(embed->inject_mod_name, name, name_length);

  // set the desired function count and allocate object buffer
  // +1 to accomodate null method def at the end
  embed->inject_func_count = function_count + 1;
  embed->inject_funcs =
      (void*)malloc(sizeof(PyMethodDef) * embed->inject_func_count);

  embed->inject_func_index = 0;
}

void py_add_injected_function(py_embed* const embed, const char* const name,
                              const char* const desc,
                              py_object (*func)(py_object, py_object)) {
  PyMethodDef* def =
      &((PyMethodDef*)(embed->inject_funcs))[embed->inject_func_index++];

  // set the method name
  size_t name_length = strlen(name);
  def->ml_name = (const char*)malloc(sizeof(char) * name_length);
  strncpy((char*)(def->ml_name), name, name_length);

  // set the method doc string
  size_t desc_length = strlen(desc);
  def->ml_doc = (const char*)malloc(sizeof(char) * desc_length);
  strncpy((char*)(def->ml_doc), desc, desc_length);

  // set method flags and function pointer
  def->ml_flags = METH_VARARGS;
  def->ml_meth = (PyObject * (*)(PyObject*, PyObject*)) func;
}

// sure, why not
static PyModuleDef injected_module;
static PyObject* get_injected_module() {
  return PyModule_Create(&injected_module);
}

void py_finish_module_injection(py_embed* const embed) {
  PyMethodDef empty_def = {NULL, NULL, 0, NULL};
  ((PyMethodDef*)(embed->inject_funcs))[embed->inject_func_count - 1] =
      empty_def;
  PyModuleDef module = {PyModuleDef_HEAD_INIT, embed->inject_mod_name, NULL, -1,
                        embed->inject_funcs};
  injected_module = module;
  PyImport_AppendInittab(embed->inject_mod_name, get_injected_module);
  // cleanup_injection_artifacts(embed);
}