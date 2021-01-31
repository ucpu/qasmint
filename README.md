# qASMint

Queued Assembler Interpreter (pronounced as `qas-mint`) is processor for simplified assembler designed for learning and optimizing less traditional algorithmic tasks.

Distinctive feature of the processor is availability of multiple stacks, queues, and tapes.

Number of available stacks, queues, tapes, their capacity and capacity of directly addressable memory pools can all be individually configured.
Individual memory pools can be marked as read only.
This makes it well suitable for simulating traditional processors, turing machines, or very limited chips.

Instructions can be disabled too, individually or whole categories.
This is useful in teaching how they work.

Every run of the processor will count number of register and memory accesses and instructions executed. This makes it awesome tool for learning algorithmic complexity and for comparing different programs solving same tasks.

# Example Programs

## Generate random numbers

This program generates 100 random numbers and prints them to output.

randgen.qasm:
```
# todo
```

## Sort numbers

This program reads all numbers from input, sorts them, and prints them to output.

sort.qasm:
```
# todo
```

## Running the programs

Example running the aforementioned programs, chained together:

`./qasmint randgen.qasm | tee random.numbers | ./qasmint sort.qasm | tee sorted.numbers`

# Processor

The qASM processor has 26 private registers (denoted as `a` through `z`), which generally have special meaning for many instructions, and 26 public registers (`A` through `Z`) which are freely available for use by programs.
Number or range of registers cannot be limited.
Each register holds one 32bit signed integer or 32bit floating point number - its interpretation depends on the instruction using it.

The processor also has up to 26 stacks (`SA` through `SZ`), up to 26 queues (`QA` through `QZ`), up to 26 tapes (`TA` through `TZ`), and up to 26 directly accessible memory pools (`MA` through `MZ`).
Their number and capacity can be restricted, for each type of structure individually, but same for all instances of that structure (except memory pools).
If no limitations are provided, the program has 4 instances of each of the structures, each with capacity of 1 million elements.
Additionally, individual memory pools can be marked read only and have different capacities of each other.
Elements stored in these structures are same as registers.

> _Example:_ You can limit a program to have 3 stacks, each with a limit of 10 elements, 5 queues, each with limit of 100 elements, and no tapes or memory pools.
But you cannot limit first stack to 100 elements and second stack to 50 elements.

> _Warning:_ Be aware of memory available in your host operating system.
All stacks, queues, and tapes are allocated as used, however, all memory pools are always allocated with full capacity.

The processor also has dedicated call stack, which cannot be directly accessed from the programs and its capacity (number of nested calls) can be limited separately.
The default limit is 1000 nested calls.

## Initialization

When starting a program:
- all public registers are initialized to zero
- *initial values of private registers are unspecified*
- all stacks and queues are empty
- all tapes, if enabled, have one element, the value of the element is zero, and the pointer is set to point to the element (position zero)
- all elements in all memory pools are initialized with zeroes, unless specified otherwise
- the call stack is initialized with a call to _main_ function

A program may also be started with some memory pools already populated with data, for example with decoded image pixels.

# Assembler

The program is read line by line.
Length of each line is restricted to about 100 characters.

The program may consist of restricted set of characters:
- alphabet: `A` through `Z` and `a` through `z` - the case is important in all places
- digits: `0` through `9`
- special characters: `-`, `+`, `.`, `_`, `#`
- characters allowed inside comments only: `*`, `/`, `,`, `(`, `)`, `?`, `!`, `:`

If line contains `#`, that character and the rest of the line is not interpreted and can be used to convey the meaning of the program.
Comments are subject to the restricted set of characters too.
There are no multi-line comments.

The program is subject to macro expansion.
Macros are generally used to provide additional instructions which provide convenient syntax for commonly used patterns.
Macro expansion is context aware - the expansion may depend on multiple tokens.
The program may NOT define new macros - they are built-in in the interpreter.
A macro may expand into multiple instructions, which may be confusing if you are counting executed instructions.
You may configure qasmint to output fully expanded program.

Generally, when multiple variants of same instruction exists, the real instructions end with `_` and the convenient function without `_`.
Directly using instructions with `_` is allowed too.

## Arithmetic instructions

## Logic instructions and conditions

## Memory access

## Flow control

### Labels

### Jumps

### Functions

## Input and output

### Numeric

### Strings

# Profiling And Tracing

You may configure `qasmint` to trace and/or profile what is the program doing.
This may be useful to debug misbehaving program or to optimize slow algorithms.

Profiling runs the program as usual and, when finished, generates separate log file with copy of the input program and with added number of executions of each instruction.

> _Note:_ Due to macro expansion, it is possible that some instructions executions accumulate together in same line.

Tracing is split into few categories, each can be enabled/disabled individually:
- reading/writing from/to registers
- reading/writing from/to structures (stacks, queues, tapes, and memory pools)
- function calls
- function arguments
- input/output
- all instructions

Tracing runs the program as usual and simultaneously logs selected actions into separate file.

> _Warning:_ Tracing may slow a program significantly due to extensive file operations and is therefore discouraged unless needed.

Both profiling and tracing can additionally be controlled from the program.
Special instructions can temporarily enable or disable tracing or profiling.
These instructions are disabled by default.
Reaching these instructions in running program, when disabled, unlike all other instruction, does not cause it to terminate and are simply ignored.
This is useful to narrow the scope in which profiling and/or tracing is effective.

