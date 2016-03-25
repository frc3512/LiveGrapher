#!/bin/bash
# skips source control metadata folders and hidden files (.*)

# format C++ code
find src -type f -name \*.cpp -o -name \*.hpp -o -name \*.inl | grep -E -v '^\.|qcustomplot' | uncrustify -c uncrustify.cfg -F - -l CPP --replace --no-backup

# format C code
find src -type f \( -name \*.c -o -name \*.h \) | grep -E -v '^\.|qcustomplot' | uncrustify -c uncrustify.cfg -F - -l C --replace --no-backup
