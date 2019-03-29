#include <stdio.h>
#include <stdlib.h>

#include "py_embed.h"

#define PY_EMBED_FAILURE 0
#define PY_EMBED_SUCCESS 1

#define PY_ERR_PRINT \
  if (PyErr_Occurred()) PyErr_Print()

#define PY_NULL_ERR_CHECK(ptr) \
  if (!ptr) {                  \
    PY_ERR_PRINT;              \
    return PY_EMBED_FAILURE;   \
  }

py_embed* create_py_embed(const char* argv[]) {
  py_embed* embed = malloc(sizeof(py_embed));
  embed->module_functions = NULL;

  // set the program name from the name of the executing binary
  embed->program_name = Py_DecodeLocale(argv[0], NULL);
  Py_SetProgramName(embed->program_name);

  Py_Initialize();

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

  PY_ERR_PRINT;

  // clean up embed state resources
  PyMem_RawFree(embed->program_name);
  destroy_py_object(embed->module);

  free(embed);

  return error_code;
}

int load_py_module(py_embed* const embed, const char* const path) {
  // if an existing module is already loaded, dispose of it
  if (embed->module) Py_DECREF(embed->module);

  embed->module = PyModule_New("embed");
  PY_NULL_ERR_CHECK(embed->module);

  PyModule_AddStringConstant(embed->module, "__file__", path);
  py_object locals = PyModule_GetDict(embed->module);
  py_object builtins = PyEval_GetBuiltins();
  PyDict_SetItemString(locals, "__builtins__", builtins);

  FILE* fp = fopen(path, "r");
  if (!fp) {
    perror("Failed to open file");
    return PY_EMBED_FAILURE;
  }

  py_object script = PyRun_FileEx(fp, path, Py_file_input, locals, locals, 1);
  if (!script) {
    PY_ERR_PRINT;
  } else {
    Py_DECREF(script);
  }

  return PY_EMBED_SUCCESS;
}

py_function load_py_function(py_embed* const embed, const char* const name) {
  if (!embed->module || !embed || !name) {
    PY_ERR_PRINT;
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

  py_object func = PyObject_GetAttrString(embed->module, name);

  if (func && PyCallable_Check(func)) {
    size_t index = embed->function_count;
    embed->module_functions[index] = func;

    ++(embed->function_count);

    return (py_function)index;
  } else {
    PY_ERR_PRINT;
    return (py_function)MODULE_FUNCTION_LOAD_FAILURE;
  }
}

py_object create_py_long(long value) {
  py_object obj = PyLong_FromLong(value);
  if (!obj) PY_ERR_PRINT;
  return obj;
}

py_object create_py_float(double value) {
  py_object obj = PyLong_FromDouble(value);
  if (!obj) PY_ERR_PRINT;
  return obj;
}

py_object create_py_bool(bool value) {
  if (value)
    Py_RETURN_TRUE;
  else
    Py_RETURN_FALSE;
}