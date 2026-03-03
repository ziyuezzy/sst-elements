#!/usr/bin/env python
#
# Copyright 2009-2025 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2025, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import os
import sys
import sst

# Add anytopo_utility directory to path
anytopo_utility = os.path.join(os.path.dirname(__file__), '..', 'topology', 'anytopo_utility')
sys.path.insert(0, anytopo_utility)

from sst.merlin.base import *
from sst.merlin.endpoint import *
from sst.merlin.interface import *
from sst.merlin.topology import *
from sst.merlin.targetgen import *

try:
    import networkx as nx
except ImportError:
    print("NetworkX is required. Please install NetworkX and try again.")
    sys.exit(1)

from HPC_topos import sf_configs
from Slimfly import SlimflyTopo

if __name__ == "__main__":

    # Simple test configuration
    UNIFIED_ROUTER_LINK_BW = 32  # Gbps
    LOAD = 0.5

    # Create Slimfly topology
    sf_topo = SlimflyTopo(*sf_configs[0])  # Use first config: (18, 5)
    sf_topo.set_endpoints_per_router(2)
    G = sf_topo.get_nx_graph()

    ### Setup the topology
    topo = topoAny()
    topo.routing_mode = "source_routing"
    topo.topo_name = "slimfly"
    topo.import_graph(G)

    # Set up the routers
    router = hr_router()
    router.link_bw = f"{UNIFIED_ROUTER_LINK_BW}Gb/s"
    router.flit_size = "8B"
    router.xbar_bw = f"{UNIFIED_ROUTER_LINK_BW * 2}Gb/s"
    router.input_latency = "20ns"
    router.output_latency = "20ns"
    router.input_buf_size = "4kB"
    router.output_buf_size = "4kB"
    router.num_vns = 2
    router.xbar_arb = "merlin.xbar_arb_lru"

    topo.router = router
    topo.link_latency = "20ns"
    topo.host_link_latency = "10ns"

    # Calculate routing table
    routing_table = topo.calculate_routing_table()

    ### Set up endpointNIC with source routing and demand matrix plugins
    endpointNIC = EndpointNIC(use_reorderLinkControl=True, topo=topo)

    # Add source routing plugin first
    endpointNIC.addPlugin("sourceRoutingPlugin", routing_table=routing_table)

    # Add DemandMatrix plugin to record traffic
    # Output is controlled by SST statistics configuration below
    endpointNIC.addPlugin("demandMatrixPlugin", metric="bytes")

    # Configure network interface parameters
    endpointNIC.link_bw = f"{UNIFIED_ROUTER_LINK_BW}Gb/s"
    endpointNIC.input_buf_size = "32kB"
    endpointNIC.output_buf_size = "32kB"
    endpointNIC.vn_remap = [0]

    # Set up target generator
    targetgen = UniformTarget()

    # Create job with offered load pattern
    ep = OfferedLoadJob(0, topo.getNumNodes())
    ep.setEndpointNIC(endpointNIC)
    ep.pattern = targetgen
    ep.offered_load = LOAD
    ep.link_bw = f"{UNIFIED_ROUTER_LINK_BW}Gb/s"
    ep.message_size = "1024B"
    ep.collect_time = "200us"
    ep.warmup_time = "200us"
    ep.drain_time = "1000us"

    system = System()
    system.setTopology(topo)
    system.allocateNodes(ep, "linear")
    system.build()

    # Enable SST statistics output to CSV with periodic sampling
    # Must be done AFTER system.build()
    sst.setStatisticLoadLevel(1)
    sst.setStatisticOutput("sst.statOutputCSV")
    sst.setStatisticOutputOptions({
        "filepath" : "demand_matrix.csv",
        "separator" : ", "
    })
    # Enable all statistics for demandMatrixPlugin component type only
    # This filters output to only show demand matrix data
    sst.enableAllStatisticsForComponentType("merlin.demandMatrixPlugin",
                                           {"type":"sst.AccumulatorStatistic","rate":"10us"})





