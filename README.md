
### What is RMCPP?

RMCPP removes C and C++ comments from source files. It is meant as a middleman between  
a Preprocessor and a Compiler. Specifically, I wrote it to track down *very* old code, including  
MODULA and Pascal code.

RMCPP supports C-style comments `/* ... */`, C++ comments `// ... `, and Modula/Pascal  
style comments `(* ... *)`, and `{ ... }` (even nested!), if run with the `--pascal` flag.

An extremely basic Macro-processor is included, but non-functional as of now.  
It is however, not meant to be a full blown standards-compliant Preprocessor, but rather  
for token replacement (it won't parse `#...` lines, is what I'm saying).

