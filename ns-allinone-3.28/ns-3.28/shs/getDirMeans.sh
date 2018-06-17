:> means.all

for d in * ; do
    if [[ -d $d ]]; then
        cd $d
        mean=`getMean.sh`
        getDirMeans.sh
        cd ..
        echo $d $mean >> means.all
        cat $d/means.all >> means.all
    fi
done