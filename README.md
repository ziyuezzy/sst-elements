This is forked by Ziyue on 24April2023. 
We will do some changes to the source code for our own usage.

Modifications by ziyue:
2024 Feb:
Modified file "./autogen.sh", such that MERLIN, FIREFLY, EMBER are compiled
Added some code in firefly to output inter-NIC traffic

2023 June:
Implemented Graph-based topologies, called 'from_graph' topology.
This new topology can import graph structure (edge list) from file (python pickled file for now) and construct the network. Another necessary file to import is the path dictionary, which stores the possible paths between any two vertices in the embedded graph. An example repository to generate these imported files is here: https://github.ugent.be/ziyzhang/topology-research (TODO: public repo instead of ugent github?)

Theoratically the 'from_graph' topology can simulate any possible network topology, however the overhead of this method is mainly the computation, disk and RAM usage for the path dictionary. This method is suitable for simulating networks that do not have specific routing rules such as Jellyfish, or the specific routing rules are difficult to define such as Slimfly, Polarfly, Equality network, etc. For simulating traditional topologies such as Torus, Fat-tree, dragonfly whose routing rules are clear and already implemented, the corresponding original Merlin topology classes are recommanded. 

2023 April:
1. Modified file "./autogen.sh", such that only MERLIN is compiled since we are only looking at MERLIN for now. We can append this list of compilation in the future. This will save compilation time and avoid wired compilation errors in other libs.
2. Modified the dragonfly topology, such that it now supports a new parameter "global_link_latency" (link latency of inter-group links). This is independent from "link_latency" (link latency of intra-group links).
3. corrected some naming of links in MESH/TORUS/SINGLE-ROUTER topo, in order to avoid some warning.
4. In offered_load.cc, I have modified the statistics of packet latency method such that it is similar to that in booksim2. Now there are two latencies: packet latency and network latency. (TODO: re-do this)
5. The uniform target generator had some defects (the last EP was not included in the pool of dest), I have corrected this.


![SST](http://sst-simulator.org/img/sst-logo-small.png)

# Structural Simulation Toolkit (SST)

#### Copyright (c) 2009-2023, National Technology and Engineering Solutions of Sandia, LLC (NTESS)
Portions are copyright of other developers:
See the file CONTRIBUTORS.TXT in the top level directory
of this repository for more information.

---

The Structural Simulation Toolkit (SST) was developed to explore innovations in highly concurrent systems where the ISA, microarchitecture, and memory interact with the programming model and communications system. The package provides two novel capabilities. The first is a fully modular design that enables extensive exploration of an individual system parameter without the need for intrusive changes to the simulator. The second is a parallel simulation environment based on MPI. This provides a high level of performance and the ability to look at large systems. The framework has been successfully used to model concepts ranging from processing in memory to conventional processors connected by conventional network interfaces and running MPI.

---

Visit [sst-simulator.org](http://sst-simulator.org) to learn more about SST.

See [SST Elements Documentation](http://sst-simulator.org/SSTPages/SSTDeveloperElementSummaryInfo/) for an overview of the simulation capabilities in this repository.

See [Contributing](https://github.com/sstsimulator/sst-elements/blob/devel/CONTRIBUTING.md) to learn how to contribute to SST.

##### [LICENSE](https://github.com/sstsimulator/sst-elements/blob/devel/LICENSE.md)
