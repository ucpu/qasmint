call ReadInputs
set L 0
copy H N
dec H # H is index of last element
call Sort
call WriteResult


# sets N to the number of elements
function ReadInputs
set N 0
label Start
readln
inv z
condreturn # no more lines to read
rstat
inv u
condreturn # cannot read unsigned integer
read U
copy i N
indstore MA U # copy the input value to i-th position in memory pool A
inc N
jump Start # go try read next input


# expects L to be index of first element
# expects H to be index of last element
function Sort
gte z L H
condreturn # empty range
call Partition
push SA M
push SA H
copy H M
call Sort
pop H SA
load M SA
push SA L
copy L M
inc L
call Sort
pop L SA
pop M SA
return


# expects L to be index of first element
# expects H to be index of last element
# sets M to some middle index
function Partition
set t 2
div l L t
div h H t
add i l h
indload P MA # P is pivot value taken at middle index between L and H
copy I L
copy J H
dec I
inc J
label First
inc I
copy i I
indload A MA
lt z A P
condjmp First
label Second
dec J
copy i J
indload B MA
gt z B P
condjmp Second
copy M J
gte z I J
condreturn
copy i I
indstore MA B
copy i J
indstore MA A
jump First


function WriteResult
set P 0
label Start
gte z P N
condreturn # all numbers written
copy i P
indload U MA # load i-th value from MA into U
write U
writeln
inc P
jump Start
