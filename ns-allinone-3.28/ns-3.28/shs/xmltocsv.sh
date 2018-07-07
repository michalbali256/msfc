discs="CoDel FqCoDel Msfc PfifoFast"
for disc in $discs; do
    grep bin < $disc$1 | perl -ne 'printf "%s\n", join(" " , $_ =~ /["(].*?[)"]/g)' | tr -d '"' | cut -d" " -f1 --complement
    echo 'NEXT'
    echo $disc
done
