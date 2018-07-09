# msfc
This repository contains my bachelor thesis: Traffic scheduler for Differentiated Services.

There is also a ns3 fork used to test the traffic scheduler.

## Simulation
The simulation immitates an ISP network. There is a tree of point-to-point connections. Each node in the tree has 1-4 children. In the leaves of the tree, there are APs of wifi. 8-12 clients are connected to each AP.
```
                          s1
                           |
                           |
                           G1
                        /  |  \
                       /   |   \
                      /    |    \
                     G2    G3   G4
                    / \  / | \ / | \ \ 
                    
                            .
                            .
                            .
                  |   |       ...       |
                 AP  AP       ...       AP 
```

All flows are between a client and server s1. All of them are simulated using OnOffApplication, which alternates on and off states. The flows are configured in msfc/ns-allinone-3.28/ns-3.28/flow_types.in. Example:
```
SSH TCP 1kbps 1 bi 20 ns3::ConstantRandomVariable[Constant=1] ns3::ConstantRandomVariable[Constant=0]
```
There are 8 fields in this order:
+ Name
+ Transport protocol (TCP/UDP)
+ Datarate
+ Priority
+ Whether it is only download or both download and upload
+ Size of one packet in bytes
+ ns3 random variable that determines on time of application
+ ns3 random variable that determines off time of application

## Prerequisities

You need a c++ compiler --- g++ or clang, and python interpreter installed.

## Build

You can build the ns3 quickly and simply by running:
```
cd msfc/ns-allinone-3.28/ns-3.28/
./waf -d optimized configure
./waf
```

For further information, see the [ns3 tutorial](https://www.nsnam.org/docs/tutorial/html/getting-started.html#building-ns3).

## Running simulation

To run a simulation, run following:
```
cd msfc/ns-allinone-3.28/ns-3.28/
./waf [--cwd=outputDirectory] --run "msfc-benchmark [arguments]"
```

There are several arguments for the benchmark, most important are:

- queueDiscType - you can choose from {PfifoFast, CoDel, Sfq, FqCoDel, Msfc}
- simDuration - duration of the simulation in seconds
- connectionDatarate - datarate of p2p links of ISP tree
- serversDatarate - datarate of p2p tree-to-server link
- randomPriority - determines whether flow priorities are determined randomly or from config file
- flowInFileName - the location of config file. Defaults to ../flow_types.in, so it is recommended to set the cwd argument to a folder in the ns-3.28 folder.
- appCount - total number of applications generating traffic installed in the whole simulation (two applications at the opposite side of bidirectional flow count as one)

An example:
```
cd msfc/ns-allinone-3.28/ns-3.28/
mkdir results
./waf --cwd=results --run "msfc-benchmark --simDuration=100  --connectionDatarate=100Mbps --serversDatarate=1000Mbps --randomPriority=0 --appCount=280 --queueDiscType=Msfc"
```

You can see all arguments of msfc-benchmark by running:
```
./waf --run "msfc-benchmark --PrintHelp"
```

## Results analysis

Overall results are in \*.all files.

.det files provide detailed information about delay and jitter of packets. A script is provided to visualise the info into violin plots. 

In folder msfc/ns-allinone-3.28/ns-3.28/shs there is plot.sh and violin.py. Move those scripts into the folder with results (or add shs folder to your PATH variable). The plot.sh script will take the .det files of various qdisc with the same suffix. The usage is as follows:

```
./plot.sh <detSuffix> <upperLimit> <detail>
```

- detSuffix is the suffix of .det files we want to process.
- upperLimit - the script will not consider values higher than this limit for the graph. However, the means are computed from all values. The values tend to be scattered from 60ms to 2s, however 99% of the values is between 60 ms and 100 ms.
- detail - detail of the graphs. Higher value means long processing (for large simulations) and nicer graphs.

Example:
```
./plot.sh -type2-delay-down.det 0.08 100
```
This compares only delays of type 2. No values higher than 0.08 are considered for the graph. Detail is 100 (quite nice).
