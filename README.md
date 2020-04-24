
### What is RMCPP?

RMCPP removes C and C++ comments from source files. It is meant as a middleman between  
a Preprocessor and a Compiler. Specifically, I wrote it to track down *very* old code, including  
MODULA and Pascal code.

RMCPP supports C-style comments `/* ... */`, C++ comments `// ... `, and Modula/Pascal  
style comments `(* ... *)`, and `{ ... }` (even nested!), if run with the `--pascal` flag.

An extremely basic Macro-processor is included, but non-functional as of now.  
It is however, not meant to be a full blown standards-compliant Preprocessor, but rather  
for token replacement (it won't parse `#...` lines, is what I'm saying).

## Synopsis

Basic usage is as follows:

    rmcpp [options] < inputfile.cpp > outputfile.cpp

or

    rmcpp [options] inputfile.cpp > outputfile.cpp

or just

    rmcpp [options] inputfile outputfile.cpp 

where [options]:

  + `-d`, `--debug` enables debugging messages. Got a file with funky-looking comments? This might help you find the issue.
  + `-w`, `--nowarnings` Disables warning messages. Right now applies to Pascal-mode when encountering nested comments, unterminated comments, and unterminated strings will trigger a warning as well.
  + `-a`, `--keepansi` keeps C comments (`/* these! */`).
  + `-c`, `--keepcpp` keeps C++ comments (`// like this one!`).
  + `-p`, `--pascal` enables Pascal mode - it will recognize Pascal-style comments, i.e., `(* these *)`, and `{ these }`, as well as C++ comments, and ANSI C comments (which are apparently used in VERY old Pascal source files).
  + `-l`, `--hash` enables deletion of basic Line comments starting with a hash symbol, i.e., `# stuff like this`.
  + `-o`, `--writecomments=<file>` writes comments removed from the input file/input stream to `<file>`.  
                                     Useful if your source also happens to be your documentation (not that i would so something like that... 😅).


## API

rmcpp is also a library, `main.cpp` shows a decent way of using it:

```c++
    CommentStripper::Options opts;
    /*
    * most parsing options are set through <CommentStripper::Options>
    * a healthy default is already set - just remove C++ and C comments.
    */
    CommentStripper crx(opts, std::cin);
    {
        /* maybe you'd like to hang on to those comments ... */
        std::fstream commfh("ancientknowledge.txt", std::ios::out | std::ios::binary);
        /* no problem! onComment() expects a bool(*)(CommentStripper::State, char) function. */
        crx.onComment([&](CommentStripper::State st, char ch)
        {
            /*
            * st is either a a comment state (CT_(kind)COMM), and if a comment has been terminated,
            * it's a CT_UNDEF.
            */
            if(st == CT_UNDEF)
            {
                commfh << std::endl;
            }
            else
            {
                commfh.put(ch);
            }
            /*
            * if you no longer want to receive comments, return false.
            * the callback reference in CommentStripper is null'd, and consequently, you
            * will no longer receive any comments. The idea here is to prevent undue memory usage,
            * in particular for very large files.
            * but for now, let's keep em coming
            */
            return true;
        });
    }
    if(crx.parse(std::cout))
    {
        std::cout << "no issues, doc!" << std::endl;
    }
    else
    {
        std::cout << "something failed to parse!" << std::endl;
    }
    /* that's it. easy-peasy. */
```

## Requirements

The main program uses [https://github.com/apfeltee/optionparser](optionparser.hpp), which is just a single header.

