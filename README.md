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
An example:
```
cd msfc/ns-allinone-3.28/ns-3.28/
mkdir results
./waf --cwd=results --run "msfc-benchmark --simDuration=100 --queueDiscType=Msfc --connectionDatarate=150Mbps --serversDatarate=500Mbps --randomPriority=0"
```

You can see all arguments of msfc-benchmark by running:
```
./waf --run "msfc-benchmark --PrintHelp"
```
