sortTraffic.sh

for d in * ; do
    if [[ -d $d ]]; then
        cd $d
        sortout.sh
        cd ..
    fi
done

sortout.sh

cd delay
sortPrios.sh
cd ..

cd goodput
sortPrios.sh
cd ..

cd RTTe
sortPrios.sh
cd ..

cd jitter
sortPrios.sh
cd ..

getDirMeans.sh