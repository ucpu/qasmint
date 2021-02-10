set I 0       # count of generated numbers
set T 1000    # count of numbers to generate
label Loop
rand J        # generate random number and store it in register J
write J       # write the number from register J to output buffer
writeln       # flush the output buffer to standard output
inc I         # increment the counter of generated numbers
lt z I T      # compare I < T and store it in z
condjmp Loop  # go generate another number if we are below the limit
