#include <stdio.h>

#include "py_embed.h"

int main(int argc, char const* argv[]) {
  // initialize the embedded Python interpreter
  py_embed* py = create_py_embed(argv);

  // load the desired module from a script
  if (!load_py_module(py, "addition.py")) {
    printf("Failed to load module\n");
    return 1;
  }

  // create some arguments
  py_object a = create_py_long(6);
  py_object b = create_py_long(10);

  // load a function from the module
  py_function add = load_py_function(py, "add");

  if (add != MODULE_FUNCTION_LOAD_FAILURE) {
    // if the function was loaded successfully, call that bad boy
    call_py_func_args_void(py, add, 2, a, b);

    // all macros:
    //
    // call_py_func_args        (Call function with arguments)
    // call_py_func             (Call function without arguments)
    // call_py_func_args_void   (Call function with arguments and dispose
    //                           of the result)
    // call_py_func_void        (Call function without arguments and dispose
    //                           of the result)
  }

  // clean up arguments
  destroy_py_object(a);
  destroy_py_object(b);

  // clean up embedded interpreter
  return destroy_py_embed(py);
}