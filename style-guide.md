TL;DR: if you modify this project, please mantain the style the code has. Some pointers at
what that style entails:

- Don't use `typedef` unless you really, really think it is going to improve on the code
    and not just to make everything a bit succinter. Check [this post on C structures][struct].
- In the documentation of each struct, list all the invariants that the struct should
    always fulfill in order to be a valid struct. Even the most simplest of invariants,
    like non-negativity, are worth mentioning.
- Create two functions for every struct you define: `is_valid_StructName` and
    `assert_valid_StructName`, where `StructName` is the name of the struct. Their purpose
    is to make sure that the invariants you defined (above) hold.
- TitleCase structs, functions are always lower case, only in CAPS are preprocessor
    variables and non-pure preprocessor functions, and enums are in `UPPERCASE_SNAKE`
    (their elements are in a weird combination of `UPPERCASE_SNAKE_and_lowc_snake`).
- Almost never use `extern`. It is tempting to define global variables, but I advise
    against it (and so do many others). It makes your code clumsy, hard to analyse,
    difficult to modularize, challenging to modify without breaking something in the way,
    so please don't do it. (Check *side note* below.)
- Use `static` for all functions that are not intended to be exported. Use `const`
    anywhere you can. Signaling that a variable is never modified makes it easier to
    future mantainers to read.
- Don't add unnecessary dependencies. Try to keep the code standalone as this will
    facilitate people in the future to compile it and run it. Don't write code that looks
    cool but will only work in your specific setup.

This project follows (somewhat) closely [mcinglis' C style guide][c-guide] and
[this post on C structures][struct]. You shall read both pages in detail as a guide into
writing good C code.

[c-guide]: https://github.com/mcinglis/c-style
[struct]: https://www.microforum.cc/blogs/entry/21-how-to-struct-lessons-on-structures-in-c/

**Side note:** I thought I somewhat understood what `static` and `extern` meant. And as
most C programmers, I venture to say, I had no idea of what they really meant. A clear
explanation of both keywords and their behaviour can be found in [this post][compilation].
I created a small questionnaire to test knowledge in this topic. If you are able to answer
all questions, you are ready to go. Good luck!

[compilation]: https://www.microforum.cc/blogs/entry/37-about-translation-units-and-how-c-code-is-compiled/

- What is a translation unit (also called compilation unit)?
- What are the (usual) stages of compiling a C program?
- What is the purpose of the header files?
- What happens if a variable is declared multiple times? What about defining it multiple
    times? Does it change if the declaration is in the global scope or inside a function?
- What are each of the three types of linkage for?
<!--(external, internal and none)-->
