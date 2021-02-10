
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
