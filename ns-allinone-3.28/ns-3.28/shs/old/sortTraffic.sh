a=`cut -d" " -f1,6 *-apps-assign.txt | tr " " -`

for i in $a; do
    id=`echo $i | cut -d"-" -f1`
    dir=`echo $i | cut -d"-" -f2`
    files=`echo * | tr " " "\n" | grep "\-$id\-"`
    mkdir $dir 2> /dev/null
    for f in $files; do
        cp $f $dir/
    done
done;