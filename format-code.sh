#!/bin/bash

# format C++ code
find $PWD ! -type d -not \( -name .svn -prune -o -name .hg -prune -o -name .git -prune -o -name ".*" -prune -o -name Makefile -prune -o -name moc_\* -prune -o -name ui_\* -prune -o -name qrc_\* -prune -o -name qcustomplot.\* -prune \) -a \( -name \*.cpp -o -name \*.hpp -o -name \*.inl \) | uncrustify -c uncrustify.cfg -F - -l CPP --replace --no-backup

# format C code
find $PWD ! -type d -not \( -name .svn -prune -o -name .hg -prune -o -name .git -prune -o -name ".*" -prune -o -name Makefile -prune -o -name moc_\* -prune -o -name ui_\* -prune -o -name qrc_\* -prune -o -name qcustomplot.\* -prune \) -a \( -name \*.c -o -name \*.h \) | uncrustify -c uncrustify.cfg -F - -l C --replace --no-backup

