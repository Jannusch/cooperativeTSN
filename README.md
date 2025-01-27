# cooperativeTSN

With cooperativeTSN we provide a simulation library that enables simustainus simulation of in-car TSN based Ethernet and the wireless connectino in between vehicles and infrastructure. It provides a flexible platform to assess how various TSN parameters, such as scheduling, traffic shaping, and redundancy, can influence the performance and reliability of wireless links in vehicular environments. Our current research focus is on utilizing TSN configurations to improve robust and efficient wireless connectivity, a key requirement for the development of advanced driver assistance systems.

This project is described in more detail in this [paper](https://www.cms-labs.org/bib/bigge2025hierarchical/). If you use this software or parts of it, it would be nice if you could cite us.

### Current Used Versions 
| Tool | Version | Status |
|----|---|---|
| OMNeT++ |  6.0.3 | unmodified |
| INET |  v4.5.2 | patched version (included in the repo) |
| Veins(_INET) | 5.3 | unmodified |
| SUMO | 1.21.0 | unmodified |
| TSNsched | 1.1 | patched |

---
## Runing the simulation
Building and running the simulation requieres a few steps as multiple libraries and different software needs to work together.
By now all the source code for the OMNeT++ simulation is in this repository.
OMNeT++ itslef as well as SUMO needs to be installed seperatly.
TSNsched is also not part of this repository.
Installation steps are provided in the TSNsched repository.

### Building the simulation
The following steps require that you have a working OMNeT++ and SUMO.
For easy setup you can use the [a docker image](dr.nsm.inf.tu-dresden.de/jannusch/wons25:latest) (this is a link to docker repo, no website).
Due to legal reasons you have to compile all the parts if you start the container for the first time:
```bash
# Build OMNeT++
cd /opt/omnetpp
./configure
make -j $(nproc)

# Go to the simulation repo
cd /opt/cooperativetsn

# Configure all the included projects (make sure omnetpp is in your PATH)
./configure

# Build the project
# We create a compile database with bear just in case you need it
bear --append -- make -j $(nproc)
```

### Runing the simulation
Make sure that TSNsched is running and you know the address of the server.
Set the values in the [`omnetpp.ini`](cooperative_tsn/examples/platoon_with_plexe/omnetpp.ini) file:
```
**.grpcAddress = $NEW_ADDRESS // default("172.17.0.3"); // network address of the scheduler (like dns name, IPv6, etc.)
**.grpcPort = $PORT // should be the default: default("50051"); // port of the gRPC server
```
Now we can start the simulation:
```bash
# Go to the default example simulation
cd cooperativeTSN/cooperative_tsn/examples/platoon_with_plexe/

# and run the simulation - best you check the run script and the OMNeT++ manual to understand which parameters can be used
./run

# some often used run commands from me
# run in debug mode with GUI
./run -d -- -u Qtenv
# run in release mode with Cmdenv and as config the Platooning-TSN-dyn config
./run -- -u Cmdenv -c Platooning-TSN-dyn
```
The results are saved in the `results` folder of the folder in which you started the simulation.

## Points of interest
Some interesting things are described in more detail in the following sections.
In the near future you can find a more complete documentation [here](https://jannusch.xyz/archive/cooperativetsn).

### Auto Gate Scheduling
The scheduling of the network is performed during the simulation, based on an initial configuration, respecting new streams during the runtime.

The approach is based on the TSNsched approach from the [inet-manual](https://inet.omnetpp.org/docs/showcases/tsn/gatescheduling/tsnsched/doc/index.html).
Instead of installing TSNsched and Z3 into this project, the scheduler is connected via gRPC.
The module and logic for this is in [`cooperative_tsn/src/cooperative_tsn/configurator/gatescheduling/`](cooperative_tsn/src/cooperative_tsn/configurator/gatescheduling/).
The gRPC client is written Python (whenever I feel like rewriting the Makefiles to add the C++ version, it will be rewritten) and located in [`cooperative_tsn/src/gRPC`](cooperative_tsn/src/gRPC).

The Scheduler itself is running in a second docker container. The original functionality of TSNsched is still there, but I added a gRPC server to it. Its in this [github repo](https://github.com/Jannusch/TSNsched).

There are several hard coded constrains in the C++ code. They are for the resent paper and solve problems with the scheduler. Hardcoding wasn't necessary, but the fastest way... and now I have to clean it up.

### TSN Nodes
The TSN nodes I used, are the "original" TSN nodes from INET. Code: [`inet/src/inet/node/tsn`](inet/src/inet/node/tsn).
The tricky part about them is the configuration, and the mixing with other modules like the Application layer module, which create the traffic or the gPTP module. 
For now I was able to store additional information about them in other modules, as I tried to modify INET as little as possible. For the energy simulation this will be changed, but as a base these nodes are fine.
In general for the use case we try to simulate, base INET should work quite well, the tricky part was the nesting of the modules, which is not necessary for the energy simulation.
Changing the parameters during the simulation is solved, but I can not point to all places where I had to change the code. The idea back in the days (TM) was to have the simulation run all the time, and change the configuration simultaneously with the configuration of the real switches. At least for the soc-e switches this should be possible, as they have the possibility to save the configuration in a file and load it from there. My idea was to parse this file in the simulation as well, and than apply it. Except for the parser, this is implemented.
Doing this the other way around, is possible as well.

### Problems
One problem in my opinion is the mismatch between the hardware and the simulation (compare https://ieeexplore.ieee.org/document/10575954).
Would be interesting to see how this effects us.





                                                                            
