---
applyTo: "src/**.c, src/**.h"
---
# About templates and sections of files
When doing edits, always respect the file template in .vscode/c.code-snippets.
There are separate templates for .h and .c files.
The templates contain sections.
Put the new code in the correct section of the file.
If a section is missing, add it.
Keep the sections in the order they are in the template.
After creating an empty file, add the appropiate template first.
Never put function definitions in header files. Only put function declarations in header files.

# About memory allocations
Never check if malloc(), calloc() or realloc() returned NULL. Assume that they always succeed. Do not add error handling for failed memory allocations.

# About comments
When creating a struct or enum, add doxygen comments to it.
When adding a function, add doxygen comments to the declaration in the header file (public function) or to the declaration in the source file, section "Private Function Declarations" (private function).

# About naming conventions
The naming convention for public functions is `moduleName_functionName` (implemented in moduleName.c).
functionName should be a verb describing the action of the function.
Use camel case for function names and variable names.
Use uppercase letters and underscores for macro names and enum values.

Do not use the ! operator for boolean conditions. Write condition == false instead.
Do not create functions, variables or defines that are not used.
Never declare multiple variables in the same line.
Make order of operations explicit with parentheses, even if they are not strictly necessary.
