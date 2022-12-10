This clang extra tool captures important information in a giving source file such as:
1) The function names.
2) The number of loops per functions (either for, while or do while loop).
3) The number of time each function is called.
4) The number of arguments per function.

This implementation leverages the libASTMatchers which is another way of traversing an AST like
the recursiveASTVisitor. In fact, the libASTMatchers provides a simple, powerful and concise way
to describe specific patterns in the AST.

The major issue encountered while implementing this tool is suppressing the hidding functions within
a source file. Failing to do so returned inaccurate results.