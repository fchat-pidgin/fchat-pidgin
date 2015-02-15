#!/usr/bin/awk -f
# This awk filter parses a Valgrind log and only keeps stack traces that
# contain the keyword "flist"

BEGIN {
    buffer=""
    printbuffer=0
    inblock=0
}

# End of a block
(NF == 1) {
    if (printbuffer == 1) print buffer
    inblock=0
    printbuffer=0
    buffer=$0
}

# Part of a block
(NF > 1) {
    inblock=1
    buffer= buffer "\n" $0
}

# If we find a keywork, keep the block for printing
(/flist/) {
    printbuffer=1
}

END {
    if (printbuffer == 1) print buffer
}
