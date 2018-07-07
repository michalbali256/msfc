#!/bin/bash
FILES=*.txt
for f in $FILES
do
  echo "Processing $f file..."
  # take action on each file. $f store current file name
  gnuplot -e filename='"'$f'"' ~/ns/ns-allinone-3.27/ns-3.27/shs/plot.gp
done