// Purpose: Print "Knock, knock, NAME", where `NAME` is a runtime supplied argument.
// Input: `NAME` (an arbitrary name string).
// Output: "Knock, knock, NAME"
// Usage: `$ matrix NAME`
// Reproduction: `$ gcc matrix.c -o build/matrix -Wall && ./build/matrix NAME`
// Reference: https://www.youtube.com/watch?v=JGoUaCmMNpE


// add standard i/o (input/output) functionality to our program
#include <stdio.h>

// `argc` = arguments count
// `argv` = arguments vector
// The  actual  names  don't  matter;  although,  they  are  widely  used  as  a
// convention.  For  example, you  could rename  them into  `argument_count` and
// `argument_vector`.
int main(int argc, char *argv[]) {
    // Check that the program was provided a name.
    // OK, but why `== 2`, and not `== 1`?{{{
    //
    // The first  argument in `argv` is  always the name of  the program itself,
    // exactly as you invoked it.  For example:
    //
    //     $ ./matrix
    //     ⇒
    //     argv[0] == './matrix'
    //
    //     $ /path/to/matrix
    //     ⇒
    //     argv[0] == '/path/to/matrix'
    //
    // That's because  the shell  splits the  command-line on  every whitespace,
    // then passes the  result to the called program; including  the name of the
    // program as you typed it.
    //
    // So, if you provide a name argument, then `argc` is 2 (1 + 1), not 1.
    //}}}
    if (argc == 2) {
        printf("Knock, knock, %s\n", argv[1]);
    } else {
        // What is `stderr`?{{{
        //
        // A pointer to the standard error output stream.
        //}}}
        // Why `fprintf()` instead of `printf()`?{{{
        //
        // `printf()` writes on `stdout`; but we want to write our error message
        // on `stderr`.  IOW, we need a  function which lets us specify where we
        // want it to be written.  That's what `fprintf()` gives us:
        //
        //     fprintf(stderr, "Usage: %s <name>\n", argv[0]);
        //             ^----^
        //             this new argument is not supported by printf()
        //
        // I *think* the "f" prefix stands for "file handle".
        //
        // See: https://stackoverflow.com/a/4628144
        //}}}
        fprintf(stderr, "Usage: %s <name>\n", argv[0]);
        // return a non-zero number to denote that an error occurred
        return 1;
    }
    // We've declared that the `main()` function should return a number:{{{
    //
    //     int main(...)
    //     ^^^
    //
    // So let's be consistent, and return a number.
    // 0 means that the program exits without error.
    //}}}
    return 0;
}
