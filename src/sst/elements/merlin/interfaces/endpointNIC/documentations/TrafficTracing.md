# Traffic Tracing Plugin

## Overview

The Traffic Tracing Plugin is a NIC plugin that logs all packet injection and ejection events to a CSV file for post-simulation traffic analysis. This is useful for analyzing network behavior, identifying hotspots, and validating routing decisions.

## Features

- **Automatic Packet ID Assignment**: Each packet is assigned a unique global ID
- **Timestamp Tracking**: Records exact simulation time (in nanoseconds) for injection and ejection
- **Thread-Safe**: Uses atomic operations for packet ID generation
- **CSV Output**: Easy-to-parse format for analysis tools

## CSV Output Format

The plugin generates a CSV file with the following columns:

```
time_ns,srcNIC,destNIC,Size_Bytes,pkt_id,event_type
```

### Column Descriptions:

- **time_ns**: Simulation time in nanoseconds when the event occurred
- **srcNIC**: Source endpoint ID (NIC that originally sent the packet)
- **destNIC**: Destination endpoint ID (NIC that will receive the packet)
- **Size_Bytes**: Packet size in bytes
- **pkt_id**: Unique packet identifier (assigned at injection time)
- **event_type**: Either "in" (injection into network) or "out" (ejection from network)

### Example Output:

```csv
time_ns,srcNIC,destNIC,Size_Bytes,pkt_id,event_type
400,40,41,46,0,in
400,67,68,46,3,in
400,100,101,46,4,in
410,40,41,46,0,out
420,67,68,46,3,out
```

## Usage

### Python Configuration

```python
from sst.merlin.base import *
from sst.merlin.endpoint import *
from sst.merlin.interface import *
from sst.merlin.topology import *

# Setup your topology
topo = topoAny()
topo.import_graph(your_graph)
routing_table = topo.calculate_routing_table()

# Create endpointNIC with traffic tracing
endpointNIC = EndpointNIC(use_reorderLinkControl=True, topo=topo)

# Add source routing plugin (if needed)
endpointNIC.addPlugin("sourceRoutingPlugin", routing_table=routing_table)

# Add traffic tracing plugin
endpointNIC.addPlugin("trafficTracingPlugin",
                     csv_filename="my_traffic_trace.csv",
                     enable_tracing=True)

# Configure the rest of your simulation...
ep = OfferedLoadJob(0, topo.getNumNodes())
ep.setEndpointNIC(endpointNIC)
```

### Configuration Parameters

- **csv_filename** (optional): Output CSV filename
  - Default: `"traffic_trace.csv"`
  - Example: `"traffic_trace_slimfly.csv"`

- **enable_tracing** (optional): Enable/disable tracing
  - Default: `True`
  - Set to `False` to disable tracing without removing the plugin

## Implementation Details

### Architecture

The TrafficTracingPlugin works by intercepting packets at two critical points:

1. **Outgoing (Injection)**: When `processOutgoing()` is called
   - Assigns a unique packet ID
   - Stores ID in packet metadata
   - Logs "in" event with current timestamp

2. **Incoming (Ejection)**: When `processIncoming()` is called
   - Retrieves packet ID from metadata
   - Logs "out" event with current timestamp

### Metadata Storage

The plugin uses the `ExtendedRequest` metadata system to store the packet ID:

```cpp
struct TrafficTracingMetadata {
    uint64_t pkt_id;
};
```

This metadata travels with the packet throughout the network, allowing the plugin to correlate injection and ejection events.

### Thread Safety

- Uses `std::atomic<uint64_t>` for packet ID generation
- Uses `std::mutex` to protect CSV file I/O operations
- Safe for multi-threaded and multi-rank SST simulations
- All file writes are protected by a lock_guard to prevent race conditions
- CSV file operations are performed immediately and flushed to ensure data persistence

### Multi-Rank Considerations

For MPI-based multi-rank simulations:
- Each rank writes to the **same shared CSV file** (if using shared filesystem)
- Mutex protection ensures thread-safe writes within a single process
- For distributed filesystems, consider using rank-specific output files:
  ```python
  # Example: One file per rank
  import sst.mpi as mpi
  rank = mpi.rank()
  endpointNIC.addPlugin("trafficTracingPlugin",
                       csv_filename=f"traffic_trace_rank{rank}.csv")
  ```
- Post-process: Merge rank-specific files after simulation completes

## Analysis Examples

### Python Analysis Script

```python
import pandas as pd

# Load the trace
df = pd.read_csv("traffic_trace.csv")

# Separate injection and ejection events
injections = df[df['event_type'] == 'in']
ejections = df[df['event_type'] == 'out']

# Calculate packet latencies
latencies = ejections.merge(injections, on='pkt_id', suffixes=('_out', '_in'))
latencies['latency_ns'] = latencies['time_ns_out'] - latencies['time_ns_in']

# Statistics
print(f"Average latency: {latencies['latency_ns'].mean():.2f} ns")
print(f"Max latency: {latencies['latency_ns'].max()} ns")
print(f"Total packets: {len(injections)}")

# Find hotspots
traffic_matrix = injections.groupby(['srcNIC', 'destNIC']).size()
print("\nTop 10 traffic flows:")
print(traffic_matrix.nlargest(10))
```

## Testing

A test script is provided: `anytopo_slimfly_tracing_test.py`

To run:

```bash
sst anytopo_slimfly_tracing_test.py
```

This will generate `traffic_trace_slimfly.csv` with traffic data from a SlimFly topology simulation.

## Performance Considerations

- **I/O Overhead**: CSV writes are flushed immediately, which may impact performance for very high packet rates
- **File Size**: Large simulations with many packets will generate large CSV files
  - Consider filtering or sampling for very long simulations
  - Post-process to binary formats if needed

## Troubleshooting

### No output file generated

- Check file write permissions in the output directory
- Verify `enable_tracing=True` in the plugin configuration
- Check SST output for error messages

### Missing packet IDs in output

- Ensure the plugin is in the correct position in the plugin pipeline
- Verify that `ExtendedRequest` is being used (automatic when using plugins)

### Duplicate packet IDs

- Should not occur due to atomic counter
- If seen, report as a bug

## Future Enhancements

Potential improvements to the traffic tracing plugin:

- [ ] Binary output format for better performance
- [ ] Sampling mode (trace only N% of packets)
- [ ] Per-endpoint output files for parallel analysis
- [ ] Additional metadata (VC, VN, hop count)
- [ ] Real-time statistics computation
- [ ] Integration with other analysis tools

## Related Files

- C++ Header: `interfaces/endpointNIC/TrafficTracing.h`
- C++ Implementation: `interfaces/endpointNIC/TrafficTracing.cc`
- Python Interface: `interfaces/pymerlin-interface.py` (TrafficTracingPlugin class)
- Test Script: `tests/anytopo_slimfly_tracing_test.py`
