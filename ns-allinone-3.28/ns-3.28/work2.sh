
for i in FqCoDel Msfc; do
    mkdir "$i"150-200
    ./waf --cwd="$i"150-200 --run "scratch/plasi --simDuration=200 --queueDiscType=$i --queueDiscSize=3000 --connectionDatarate=150Mbps --serversDatarate=200Mbps"
    mkdir "$i"200-200
    ./waf --cwd="$i"200-200 --run "scratch/plasi --simDuration=200 --queueDiscType=$i --queueDiscSize=4000 --connectionDatarate=150Mbps --serversDatarate=300Mbps"
    mkdir "$i"300-500
    ./waf --cwd="$i"300-500 --run "scratch/plasi --simDuration=200 --queueDiscType=$i --queueDiscSize=6000 --connectionDatarate=300Mbps --serversDatarate=500Mbps"
    mkdir "$i"200-900
    ./waf --cwd="$i"200-900 --run "scratch/plasi --simDuration=200 --queueDiscType=$i --queueDiscSize=4000 --connectionDatarate=200Mbps --serversDatarate=900Mbps"
    
done

mkdir msfc5000
./waf --cwd=msfc5000 --run "scratch/plasi --simDuration=200 --queueDiscType=Msfc --queueDiscSize=3000 --connectionDatarate=150Mbps --serversDatarate=200Mbps --msfcMultiplier=5000"
