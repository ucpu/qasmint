set Y 0
label Outer
set X 0
label Inner
call Pixel
inc X
lt z X W
condjmp Inner
inc Y
lt z Y H
condjmp Outer


# expects X and Y be coordinates of the pixel
function Pixel

# P = (Y * W + X) * C
mul P Y W
add P P X
mul P P C
fset S 0

set I 0
label Loop
add i P I
indload V MA
fadd S S V
inc I
lt z I C
condjmp Loop

u2f D C
fdiv S S D

set I 0
label Loop2
add i P I
indstore MA S
inc I
lt z I C
condjmp Loop2

return
