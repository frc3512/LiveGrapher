#!/bin/bash
# skips source control metadata folders and hidden files (.*)

FOLDERS="common host src"

# format C++ code
for dir in $FOLDERS;
do
  find $dir -type f -name \*.cpp -o -name \*.hpp -o -name \*.inl | grep -E -v '^\.|qcustomplot' | uncrustify -c uncrustify.cfg -F - -l CPP --replace --no-backup
done

# format C code
for dir in $FOLDERS;
do
  find src -type f \( -name \*.c -o -name \*.h \) | grep -E -v '^\.|qcustomplot' | uncrustify -c uncrustify.cfg -F - -l C --replace --no-backup
done
