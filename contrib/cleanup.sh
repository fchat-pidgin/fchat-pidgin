#!/bin/sh

for file in $@; do
  dos2unix $file
  sed -i $file -e 's/[ \t]*$//'
done
