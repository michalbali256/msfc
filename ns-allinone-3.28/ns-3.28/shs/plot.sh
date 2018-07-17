#!/bin/bash

#discs="CoDel FqCoDel Msfc PfifoFast"
discs="FqCoDel Msfc"
{
echo "$1"
for disc in $discs; do
    grep bin < "$disc-$1" | perl -ne 'printf "%s\n", join(" " , $_ =~ /["(].*?[)"]/g)' | tr -d '"' | cut -d" " -f1 --complement
    echo 'NEXT' $disc
    
done; } | violin.py "$2" "$3"
