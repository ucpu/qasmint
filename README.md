# qASMint

Queued Assembler Interpreter (pronounced as `qas-mint`) is processor simulator for simplified assembler designed for learning and optimizing less traditional algorithmic tasks.

Distinctive feature of the processor is availability of multiple stacks, queues, and tapes.

Number of available stacks, queues, tapes, their capacity and capacity of directly addressable memory pools can all be individually configured.
Individual memory pools can be marked as read only.
This makes it well suitable for simulating traditional processors, turing machines, or very limited chips.

Instructions can be disabled too, individually or whole categories.
This is useful in teaching how they work.

Every run of the processor will count instructions executed.
This makes it awesome tool for learning algorithmic complexity and for comparing different programs solving same tasks.

The qASM compiler and cpu simulator are implemented in library that can be linked in your own application.

# Example Programs

## Generate random numbers

This program generates 1000 random numbers and prints them to output.

randgen.qasm:
```bash
set I 0       # count of generated numbers
set T 1000    # count of numbers to generate
label Loop
rand J        # generate random number and store it in register J
write J       # write the number from register J to output buffer
writeln       # flush the output buffer to standard output
inc I         # increment the counter of generated numbers
lt z I T      # compare I < T and store it in z
condjmp Loop  # go generate another number if we are below the limit
```

## Sort numbers

This program reads all numbers from input, sorts them, and prints them to output.

sort.qasm:
```bash
# read input
set C 0             # number of elements
label InputBegin
readln              # read one line from standard input into input buffer
inv z               # invert the flag whether we succeeded reading a line
condjmp SortBegin   # start sorting if there are no more numbers to read
label Input
rstat               # check what is in the input buffer
copy z u            # is it unsigned integer?
inv z               # is it NOT unsigned integer?
condjmp InvalidInput # handle invalid input
read V              # read the number into register V
store TA V          # store the number from the register V onto the current position on tape A
right TA            # move the position (the read/write head) on tape A one element to the right
inc C               # increment the counter of numbers
jump InputBegin     # go try read another number
label InvalidInput
terminate           # nothing useful to do with invalid input

# start a sorting pass over all elements
# this is the outer loop in bubble sort
label SortBegin
set M 0             # flag if anything has been modified?
center TA           # move the head on tape A to the initial position
right TA            # and one element to the right

# compare two elements and swap them if needed
# this is the inner loop in bubble sort
label Sorting
stat TA             # retrieve information about the tape A
gte z p C           # compare head position on tape A with value in register C and store it in register z
condjmp Ending      # jump to Ending if z evaluates true
left TA             # move the head one left
load L TA           # load value from tape A to register L
right TA            # move the head back
load R TA           # load value to register R
lte z L R           # compare values in registers L and R and store the result in register Z
condjmp NextPair    # skip some instructions if z is true
store TA L          # store value from register L onto the tape
left TA             # move head one left again
store TA R          # store value from R
right TA            # and move the head back again
set M 1             # mark that we made a change
label NextPair
right TA            # move head one right - this is the first instruction to execute after the skip
jump Sorting        # go try sort next pair of elements

# finished a pass over all elements
label Ending
copy z M            # copy value from register M to register z
condjmp SortBegin   # go start another sorting pass if we made any modifications

# write output
center TA           # move the head on tape A to the initial position
set Z 0             # count of outputted numbers
label Output
gte z Z C           # do we have more numbers to output?
condjmp Done        # no, we do not
load V TA           # load number from the tape A into register V
write V             # write value from register V into output buffer
writeln             # flush the output buffer to standard output
right TA            # move the head on tape A one element to the right
inc Z               # increment count of outputted numbers
jump Output         # go try output another number

label Done
```

## Running the programs

Example running the aforementioned programs, chained together:

```bash
./qasmint -f -p randgen.qasm | tee random.numbers | ./qasmint -f -p sort.qasm | tee sorted.numbers
```

- `-f` - suppresses logging to standard output (only output from the qasm program is printed)
- `-p` - path to file with source code of the program
- `tee` - standard linux program to duplicate its input to both file and its own standard output - it is used here to allow examining the numbers

# Processor

The qASM processor has 26 implicit registers (denoted as `a` through `z`), which generally have special meaning for many instructions, and 26 explicit registers (`A` through `Z`) which are freely available for use by programs.
Number or range of registers cannot be limited.
Each register holds one 32bit signed or unsigned integer or 32bit floating point number - its interpretation depends on the instruction using it.

The processor also has up to 26 stacks (`SA` through `SZ`), up to 26 queues (`QA` through `QZ`), up to 26 tapes (`TA` through `TZ`), and up to 26 directly accessible memory pools (`MA` through `MZ`).
Their number and capacity can be restricted, for each type of structure individually, but same for all instances of that structure (except memory pools).
If no limitations are provided, the program has 4 instances of each of the structures, each with capacity of 1 million elements.
Additionally, individual memory pools can be marked read only and have different capacities of each other.
Elements in these structures hold same type as registers (you cannot directly access individual bytes).

> _Example:_ You can limit a program to have 3 stacks, each with a limit of 10 elements, 5 queues, each with limit of 100 elements, and no tapes or memory pools.
But you cannot limit first stack to 100 elements and second stack to 50 elements.

> _Warning:_ Be aware of memory available in your host operating system.
All stacks, queues, and tapes are allocated as used, however, all memory pools are always allocated with full capacity.

The processor also has dedicated call stack, which cannot be directly accessed from the programs and its capacity (number of nested calls) can be limited separately.
The default limit is 1000 nested calls.

## Initialization

When starting a program:
- all explicit registers are initialized to zero, unless specified otherwise
- *initial values of implicit registers are unspecified*
- all stacks and queues are empty
- all tapes, if enabled, have one element, the value of the element is zero, and the pointer is set to point to the element (position zero)
- all elements in all memory pools are initialized with zeroes, unless specified otherwise

A program may also be started with some explicit registers and some memory pools already populated with data, for example with decoded image pixels.

# Assembler

The program is read line by line.
Each line is limited to about 100 characters.
(Line ends are automatically converted.)

The program may consist of restricted set of characters:
- alphabet: `A` through `Z` and `a` through `z` - the case is important
- digits: `0` through `9`
- special characters: space, `-`, `+`, `.`, `_`, `@`, `#`
- characters allowed inside comments only: `*`, `/`, `,`, `(`, `)`, `<`, `>`, `=`, `?`, `!`, `:`, `;`

If line contains `#`, that character and the rest of the line is not interpreted and can be used to convey the meaning of the program.
Comments are subject to the restricted set of characters too.
There are no multi-line comments.

Empty lines are ignored.

## Placeholders

Instructions descriptions in the following chapters use [brackets] to denote placeholders, which should be replaced by actual values in the programs.

Placeholders can be replaced by a name of a register (eg. `z` or `K`).
Some instructions allow use of a structure (eg. stack `SS` or queue `QB`).
Additionally, memory pools may have address specified (eg. `MC@42` - which denotes 42nd element of the C memory pool).

> _Note:_ Some instructions can be used with some structures or registers only.
This is usually indicated further in the description by highlighting the type.

## Register instructions

*reset* [dst] - set register [dst] to zero.

*set* [dst] [literal] - set register [dst] to the unsigned integer [literal].

*iset* [dst] [literal] - set register [dst] to the signed integer [literal].

*fset* [dst] [literal] - set register [dst] to the floating point number [literal].

*copy* [dst] [src] - copy value from register [src] into register [dst].

*condrst* [dst] - if value in `z` evaluates true, set register [dst] to zero, otherwise do nothing.

*condset* [dst] [literal] - if value in `z` evaluates true, set register [dst] to the unsigned integer [literal], otherwise do nothing.

*condiset* [dst] [literal] - if value in `z` evaluates true, set register [dst] to the signed integer [literal], otherwise do nothing.

*condfset* [dst] [literal] - if value in `z` evaluates true, set register [dst] to the floating point number [literal], otherwise do nothing.

*condcpy* [dst] [src] - if value in `z` evaluates true, copy value from register [src] into register [dst], otherwise do nothing.

*indcpy* - copy value from register whose index is read from `s`, into register whose index is read from `d`.
Explicit registers (A-Z) are indexed 0 through 25 and implicit register (a-z) are indexed 26 through 51.
This instruction terminates the program if either of the indices is invalid.

## Arithmetic instructions

Overflows, underflows etc. are silently ignored.

*add*, *sub*, *mul*, *div*, *mod* [dst] [left] [right] - unsigned integer arithmetic operations, reads its parameters from [left] and [right] and stores the result in [dst].
Division by zero terminates the program.

*inc*, *dec* [dst] - unsigned integer arithmetic operations, update value in [dst] in place.

*iadd*, *isub*, *imul*, *idiv*, *imod* [dst] [left] [right] - signed integer arithmetic operations, reads its parameters from [left] and [right] and stores the result in [dst].
Division by zero terminates the program.

*iinc*, *idec* [dst] - signed integer arithmetic operations, update value in [dst] in place.

*iabs* [dst] [src] - signed integer arithmetic operations, reads its parameter from [src] and stores the result in [dst].

*fadd*, *fsub*, *fmul*, *fdiv*, *fpow*, *fatan2* [dst] [left] [right] - floating point arithmetic operations, reads its parameters from [left] and [right] and stores the result in [dst].

*fabs*, *fsqrt*, *flog*, *fsin*, *fcos*, *ftan*, *fasin*, *facos*, *fatan*, *ffloor*, *fround*, *fceil* [dst] [src] - floating point arithmetic operations, reads its parameter from [src] and stores the result in [dst].

*s2f*, *u2f* [dst] [src] - converts signed or unsigned integer to floating point number.

*f2s*, *f2u* [dst] [src] - converts floating point number to signed or unsigned integer.

## Logic instructions

All logic instructions operate on unsigned integers.

*and*, *or*, *xor* [dst] [left] [right] - reads its parameters from [left] and [right], converts both parameters to boolean before applying the operation, and stores the result in [dst].

*not* [dst] [src] - reads its parameter from [src], converts the parameter to boolean before applying the operation, and stores the result in [dst].

*inv* [dst] - value in [dst] is converted to boolean and inverted in place.

*shl*, *shr*, *rol*, *ror* [dst] [left] [right] - reads its parameters from [left] and [right], performs left/right shift/rotation, and stores the result in [dst].

*band*, *bor*, *bxor* [dst] [left] [right] - reads its parameters from [left] and [right], applies the logic operation to all bits individually, and stores the result in [dst].

*bnot* [dst] [src] - reads its parameter from [src], applies the logic operation to all bits individually, and stores the result in [dst].

*binv* [dst] - all bits in [dst] are inverted in place.

## Comparisons

*eq*, *neq*, *lt*, *gt*, *lte*, *gte* [dst] [left] [right] - reads its parameters from [left] and [right], applies unsigned integer comparison, and stores the result in [dst].

*ieq*, *ineq*, *ilt*, *igt*, *ilte*, *igte* [dst] [left] [right] - reads its parameters from [left] and [right], applies signed integer comparison, and stores the result in [dst].

*feq*, *fneq*, *flt*, *fgt*, *flte*, *fgte* [dst] [left] [right] - reads its parameters from [left] and [right], applies floating point comparison, and stores the result in [dst].

*fisnan*, *fisinf*, *fisfin*, *fisnorm* [dst] [src] - reads its parameter from [src], and stores the result in [dst].

*test* [dst] [src] - reads its parameter from [src], does integer comparison, and stores the result in [dst].
If the value is zero, the result is 0, otherwise 1.

## Memory structures

*load* [dst] [src] - reads value from [src], which may be top cell of a stack, _front_ cell of a queue, cell at current position on a tape or a cell in a memory pool, and stores it into register [dst].
Use @-notation to define the address when accessing memory pool.
This instruction terminates the program if the cell does not exists.
This instruction does NOT remove elements from the structures.

*indload* [dst] [src] - read `i`-th cell from *memory pool* [src] and store it in [dst].
This instruction terminates the program if the cell does not exists.

*indindload* [dst] - read `i`-th cell from `j`-th memory pool and store it in [dst].
This instruction terminates the program if the cell does not exists.

*store* [dst] [src] - reads value from register [src] and stores it in the top cell of a stack, _front_ cell of a queue, cell at current position on a tape or a cell in memory pool.
Use @-notation to define the address when accessing memory pool.
This instruction terminates the program if the cell does not exists or is read only.
This instruction does NOT add new elements to the structures.

*indstore* [dst] [src] - write value from [src] into `i`-th cell in *memory pool* [dst].
This instruction terminates the program if the cell does not exists or is read only.

*indindstore* [src] - write value from [src] into `i`-th cell in `j`-th memory pool.
This instruction terminates the program if the cell does not exists or is read only.

*pop* [dst] [src] - reads value from top cell of *stack* [src], stores it in register [dst], and removes the element from the stack.
This instruction terminates the program if the stack is disabled or empty.

*push* [dst] [src] - creates new element on top of the *stack* [dst], and stores the value from register [src] into it.
This instruction terminates the program if the stack is disabled or full.

*dequeue* [dst] [src] - reads value from front cell of *queue* [src], stores it in register [dst], and removes the element from the queue.
This instruction terminates the program if the queue is disabled or empty.

*enqueue* [dst] [src] - creates new element on back of the *queue* [dst], and stores the value from register [src] into it.
This instruction terminates the program if the queue is disabled or full.

*left*, *right* [dst] - moves the pointer on *tape* [dst] to left/right by 1 element.
These instructions terminate the program if the tape is disabled or the move would surpass its capacity.

*center* [dst] - centers the pointer on *tape* [dst] to the initial element (position zero).
This instruction terminates the program if the tape is disabled.

*swap* [left] [right] - swap structures [left] and [right].
The structures must be of same type, otherwise the program is ill formed.

*indswap* [src] - swap `i`-th instance of structure [src] and `j`-th instance of structure [src].
This instruction terminates the program if either of the structures do not exists.

*stat* [src] - retrieves informations about the structure [src].
Set register `e` whether the structure is enabled.
Set register `a` whether any elements in the structure are available (non-empty).
Set register `f` whether the structure is full.
Set register `w` whether the structure is writable (not read only).
Set register `c` to the capacity of the structure.
Set register `s` to current size of the structure.
Set register `p` to current position on a tape.
Set register `l` to leftmost index of a valid element on a tape.
Set register `r` to rightmost index of a valid element on a tape.

*indstat* [src] - retrieves informations about `i`-th instance of structure [src].

## Jumps

*label* [name] - defines point in code which other instructions can jump to by name.
The name must start with `A` through `Z`, may contain `a` through `z`, `A` through `Z`, and `0` through `9` only, and must be at least 3 and at most 20 characters long.
Labels are scoped within their function, and must be unique in their scope.

*jump* [label] - unconditionally jumps to [label].

*condjmp* [label] - if value in `z` evaluates true, jump to [label], otherwise do nothing.

No jumps may cross function boundaries.

## Functions

*function* [name] - defines starting point of a function, which can be called into by name.
The name must start with `A` through `Z`, may contain `a` through `z`, `A` through `Z`, and `0` through `9` only, and must be at least 3 and at most 20 characters long.
Function names must be unique.
This instruction ends the scope of previous function and starts scope of this function.

*call* [function] - pushes next instruction pointer onto call stack and jumps to first instruction in the function [function].

*condcall* [function] - if value in `z` evaluates true, push next instruction pointer onto call stack and jump to first instruction in the function [function], otherwise do nothing.

*return* - pops instruction pointer from call stack and jumps to it.

*condreturn* - if value in `z` evaluates true, pop instruction pointer from call stack and jump to it, otherwise do nothing.

Function calls do not store any other registers in the stack - the function itself is responsible for not corrupting the state of the calling function.
The convention is that implicit registers may be freely changed whereas explicit registers should always be restored to original values before returning from function.
It is best to use explicit registers for passing arguments and returning values.

Leaving function scope without returning terminates the program.

## Input and output

Input and output streams are cached line by line.
Following instructions can manipulate input and output line buffers.

*rstat*, *wstat* - retrieves information about current line in input or output stream.
Set register `u` whether reading/writing unsigned integer is possible.
Set register `i` whether reading/writing signed integer is possible.
Set register `f` whether reading/writing floating point number is possible.
Set register `c` whether reading/writing a character is possible.
Set register `w` whether current character is white character (eg. space).
Set register `s` to current size of the buffer.
Set register `p` to current position in the buffer.

*read* [dst] - read unsigned integer from input buffer and store it in [dst].
This instruction terminates the program if input is invalid.

*iread* [dst] - read signed integer from input buffer and store it in [dst].
This instruction terminates the program if input is invalid.

*fread* [dst] - read floating point number from input buffer and store it in [dst].
This instruction terminates the program if input is invalid.

*cread* [dst] - read single character from input buffer and store it in [dst].
The character read is ASCII encoded.
This instruction terminates the program if input is invalid.

*readln* - clear input buffer and fill it with new line from standard input.
The buffer is filtered with allowed characters only.
Set register `f` whether any filtering has happened.
Set register `z` whether the line was successfully read from the input.

*rreset* - reset position in input buffer back to beginning.

*rclear* - clear input buffer.

*write* [src] - write unsigned integer from [src] into output buffer.
This instruction terminates the program if the buffer is full.

*iwrite* [src] - write signed integer from [src] into output buffer.
This instruction terminates the program if the buffer is full.

*fwrite* [src] - write floating point number from [src] into output buffer.
This instruction terminates the program if the buffer is full.

*cwrite* [src] - write single character from [src] into output buffer.
The character written is ASCII encoded and must correspond to one of allowed characters.
The program is terminated if the character is not allowed.
This instruction terminates the program if the buffer is full.

*writeln* - append line feed to output buffer and flush it to standard output.
Set register `z` whether the line was successfully written to the output.

*wreset* - reset position in output buffer back to beginning.

*wclear* - clear output buffer.

*rwswap* - swap current input and output buffers.

## Random

*rand* [dst] - generate random unsigned integer and store it in [dst].

*irand* [dst] - generate random signed integer and store it in [dst].

*frand* [dst] - generate random floating point number in range 0 (inclusive) to 1 (exclusive) and store it in [dst].

## Miscelaneous

*profiling* [literal] - disable profiling if the unsigned integer [literal] is zero and enable it otherwise.
This instruction is disabled by default.
Reaching this instruction when disabled does not terminate the program.

*tracing* [literal] - disable tracing if the unsigned integer [literal] is zero and enable it otherwise.
This instruction is disabled by default.
Reaching this instruction when disabled does not terminate the program.

*breakpoint* - explicitly request program interruption.
This instruction is disabled by default.
Reaching this instruction when disabled does not terminate the program.

*terminate* - explicitly request program termination.

# Profiling And Tracing

You may configure `qasmint` to trace and/or profile what is the program doing.
This may be useful to debug misbehaving program or to optimize slow algorithms.

Profiling runs the program as usual and, when finished, generates separate log file with copy of the input program and with added number of executions of each instruction.

Tracing is split into few categories, each can be enabled/disabled individually:
- reading/writing from/to registers
- reading/writing from/to structures (stacks, queues, tapes, and memory pools)
- function calls
- function arguments
- input/output
- all instructions

Tracing runs the program as usual and simultaneously logs selected actions into separate file.

> _Warning:_ Tracing may slow the program significantly and is therefore discouraged unless needed.

Both profiling and tracing can additionally be controlled from the program.
Special instructions can temporarily enable or disable tracing or profiling.
These instructions are disabled by default.
Reaching these instructions in running program, when disabled, unlike other instruction, does not cause it to terminate and are simply ignored.
This is useful to narrow the scope in which profiling and/or tracing is effective.

