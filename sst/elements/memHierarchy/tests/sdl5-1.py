# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "300000ns")

# Define the simulation components
comp_cpu0 = sst.Component("cpu0", "memHierarchy.trivialCPU")
comp_cpu0.addParams({
      "workPerCycle" : """1000""",
      "commFreq" : """100""",
      "memSize" : """0x1000""",
      "num_loadstore" : """1000""",
      "do_write" : """1"""
})
comp_c0_l1cache = sst.Component("c0.l1cache", "memHierarchy.Cache")
comp_c0_l1cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """4 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """4""",
      "access_latency_cycles" : """5""",
      "low_network_links" : """1""",
      "cache_line_size" : """64""",
      "L1" : """1""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1"""
})
comp_cpu1 = sst.Component("cpu1", "memHierarchy.trivialCPU")
comp_cpu1.addParams({
      "workPerCycle" : """1000""",
      "commFreq" : """100""",
      "memSize" : """0x1000""",
      "num_loadstore" : """1000""",
      "do_write" : """1"""
})
comp_c1_l1cache = sst.Component("c1.l1cache", "memHierarchy.Cache")
comp_c1_l1cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """4 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """4""",
      "access_latency_cycles" : """5""",
      "low_network_links" : """1""",
      "cache_line_size" : """64""",
      "L1" : """1""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1"""
})
comp_n0_bus = sst.Component("n0.bus", "memHierarchy.Bus")
comp_n0_bus.addParams({
      "bus_frequency" : """2 Ghz"""
})
comp_n0_l2cache = sst.Component("n0.l2cache", "memHierarchy.Cache")
comp_n0_l2cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """32 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """8""",
      "access_latency_cycles" : """20""",
      "low_network_links" : """1""",
      "cache_line_size" : """64""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1""",
      "high_network_links" : """1"""
})
comp_cpu2 = sst.Component("cpu2", "memHierarchy.trivialCPU")
comp_cpu2.addParams({
      "workPerCycle" : """1000""",
      "commFreq" : """100""",
      "memSize" : """0x1000""",
      "num_loadstore" : """1000""",
      "do_write" : """1"""
})
comp_c2_l1cache = sst.Component("c2.l1cache", "memHierarchy.Cache")
comp_c2_l1cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """4 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """4""",
      "access_latency_cycles" : """5""",
      "low_network_links" : """1""",
      "cache_line_size" : """64""",
      "L1" : """1""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1"""
})
comp_cpu3 = sst.Component("cpu3", "memHierarchy.trivialCPU")
comp_cpu3.addParams({
      "workPerCycle" : """1000""",
      "commFreq" : """100""",
      "memSize" : """0x1000""",
      "num_loadstore" : """1000""",
      "do_write" : """1"""
})
comp_c3_l1cache = sst.Component("c3.l1cache", "memHierarchy.Cache")
comp_c3_l1cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """4 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """4""",
      "access_latency_cycles" : """5""",
      "low_network_links" : """1""",
      "cache_line_size" : """64""",
      "L1" : """1""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1"""
})
comp_n1_bus = sst.Component("n1.bus", "memHierarchy.Bus")
comp_n1_bus.addParams({
      "bus_frequency" : """2 Ghz"""
})
comp_n1_l2cache = sst.Component("n1.l2cache", "memHierarchy.Cache")
comp_n1_l2cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """32 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """8""",
      "access_latency_cycles" : """20""",
      "low_network_links" : """1""",
      "cache_line_size" : """64""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1""",
      "high_network_links" : """1"""
})
comp_n2_bus = sst.Component("n2.bus", "memHierarchy.Bus")
comp_n2_bus.addParams({
      "bus_frequency" : """2 Ghz"""
})
comp_l3cache = sst.Component("l3cache", "memHierarchy.Cache")
comp_l3cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """64 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """8""",
      "access_latency_cycles" : """100""",
      "low_network_links" : """1""",
      "cache_line_size" : """64""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1""",
      "high_network_links" : """1"""
})
comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
      "coherence_protocol" : """MSI""",
      "debug" : """${MEM_DEBUG}""",
      "access_time" : """1000 ns""",
      "mem_size" : """512""",
      "clock" : """1GHz""",
      "backend" : """memHierarchy.dramsim""",
      "device_ini" : """DDR3_micron_32M_8B_x4_sg125.ini""",
      "system_ini" : """system.ini"""
})


# Define the simulation links
link_bus_l3cache = sst.Link("link_bus_l3cache")
link_bus_l3cache.connect( (comp_n2_bus, "low_network_0", "10000ps"), (comp_l3cache, "high_network_0", "10000ps") )
link_c0_l1cache_l2cache_link = sst.Link("link_c0_l1cache_l2cache_link")
link_c0_l1cache_l2cache_link.connect( (comp_c0_l1cache, "low_network_0", "10000ps"), (comp_n0_bus, "high_network_0", "10000ps") )
link_c1_l1cache_l2cache_link = sst.Link("link_c1_l1cache_l2cache_link")
link_c1_l1cache_l2cache_link.connect( (comp_c1_l1cache, "low_network_0", "10000ps"), (comp_n0_bus, "high_network_1", "10000ps") )
link_c2_l1cache_l2cache_link = sst.Link("link_c2_l1cache_l2cache_link")
link_c2_l1cache_l2cache_link.connect( (comp_c2_l1cache, "low_network_0", "10000ps"), (comp_n1_bus, "high_network_0", "10000ps") )
link_c3_l1cache_l2cache_link = sst.Link("link_c3_l1cache_l2cache_link")
link_c3_l1cache_l2cache_link.connect( (comp_c3_l1cache, "low_network_0", "10000ps"), (comp_n1_bus, "high_network_1", "10000ps") )
link_cpu0_l1cache_link = sst.Link("link_cpu0_l1cache_link")
link_cpu0_l1cache_link.connect( (comp_cpu0, "mem_link", "1000ps"), (comp_c0_l1cache, "high_network_0", "1000ps") )
link_cpu1_l1cache_link = sst.Link("link_cpu1_l1cache_link")
link_cpu1_l1cache_link.connect( (comp_cpu1, "mem_link", "1000ps"), (comp_c1_l1cache, "high_network_0", "1000ps") )
link_cpu2_l1cache_link = sst.Link("link_cpu2_l1cache_link")
link_cpu2_l1cache_link.connect( (comp_cpu2, "mem_link", "1000ps"), (comp_c2_l1cache, "high_network_0", "1000ps") )
link_cpu3_l1cache_link = sst.Link("link_cpu3_l1cache_link")
link_cpu3_l1cache_link.connect( (comp_cpu3, "mem_link", "1000ps"), (comp_c3_l1cache, "high_network_0", "1000ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l3cache, "low_network_0", "10000ps"), (comp_memory, "direct_link", "10000ps") )
link_n0_bus_l2cache = sst.Link("link_n0_bus_l2cache")
link_n0_bus_l2cache.connect( (comp_n0_bus, "low_network_0", "10000ps"), (comp_n0_l2cache, "high_network_0", "1000ps") )
link_n0_l2cache_l3cache = sst.Link("link_n0_l2cache_l3cache")
link_n0_l2cache_l3cache.connect( (comp_n0_l2cache, "low_network_0", "10000ps"), (comp_n2_bus, "high_network_0", "10000ps") )
link_n1_bus_l2cache = sst.Link("link_n1_bus_l2cache")
link_n1_bus_l2cache.connect( (comp_n1_bus, "low_network_0", "10000ps"), (comp_n1_l2cache, "high_network_0", "1000ps") )
link_n1_l2cache_l3cache = sst.Link("link_n1_l2cache_l3cache")
link_n1_l2cache_l3cache.connect( (comp_n1_l2cache, "low_network_0", "10000ps"), (comp_n2_bus, "high_network_1", "10000ps") )
# End of generated output.