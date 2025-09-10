#include <sst_config.h>
#include "anytopo.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdlib.h>
#include <set>
#include "sst/core/rng/xorshift.h"
#include "sst/core/output.h"

using namespace SST::Merlin;

topo_any::topo_any(ComponentId_t cid, Params& params, int num_ports, int rtr_id, int num_vns): 
    Topology(cid), router_id(rtr_id), num_vns(num_vns), output(getSimulationOutput()){

    num_routers = params.find<int>("num_routers", 1);
    num_R2N_ports = params.find<int>("num_R2N_ports", 1);
    num_R2R_ports = params.find<int>("num_R2R_ports", 1);
    if (num_R2R_ports != num_ports - num_R2N_ports){
        fatal(CALL_INFO, -1, "Number of ports mismatch");
    }
    
    vns = new vn_info[num_vns];

    // Read parameters for vn_info
    std::vector<std::string> vn_routing_mode = read_array_param<std::string>(params, "routing_mode", num_vns, "source_routing");
    std::vector<int> vcs_per_vn = read_array_param<int>(params, "vcs_per_vn", num_vns, default_vcs_per_vn);
    std::vector<int> vn_ugal_bias = read_array_param<int>(params, "vn_ugal_bias", num_vns, 50);
    std::vector<std::string> vn_dest_tag_algo = read_array_param<std::string>(params, "dest_tag_routing_algo", num_vns, "none");

    // Set up VN info
    int start_vc = 0;
    for (int i = 0; i < num_vns; ++i) {
        vns[i].routing_mode = (vn_routing_mode[i] == "source_routing") ? source_routing : dest_tag_routing;
        vns[i].vn_dest_tag_routing_algo = none;
        vns[i].start_vc = start_vc;
        vns[i].num_vcs = vcs_per_vn[i];
        vns[i].ugal_bias = vn_ugal_bias[i];
        start_vc += vcs_per_vn[i];

        // If destination tag routing , set the algorithm
        if (vns[i].routing_mode == dest_tag_routing) {
            if (vn_dest_tag_algo[i] == "nonadaptive") vns[i].vn_dest_tag_routing_algo = nonadaptive;
            else if (vn_dest_tag_algo[i] == "adaptive") vns[i].vn_dest_tag_routing_algo = adaptive;
            else if (vn_dest_tag_algo[i] == "ECMP") vns[i].vn_dest_tag_routing_algo = ECMP;
            else if (vn_dest_tag_algo[i] == "UGAL") vns[i].vn_dest_tag_routing_algo = UGAL;
            else fatal(CALL_INFO, -1, "ERROR: Invalid destination tag routing algorithm: %s\n", vn_dest_tag_algo[i].c_str());
        }
    }

    // Initialize RNG
    rng = new RNG::XORShiftRNG(rtr_id+1);

    Parse_routing_info(params);
}

// Helper function to parse a delimited parameter string into a vector of vector of strings
static std::vector<std::vector<std::string>> parse_param_entries(const std::string& param_str, char entry_delim = ';', char token_delim = ',') {
    std::vector<std::vector<std::string>> entries;
    std::istringstream param_stream(param_str);
    std::string entry;
    while (std::getline(param_stream, entry, entry_delim)) {
        // Trim whitespace
        entry.erase(0, entry.find_first_not_of(" \t"));
        entry.erase(entry.find_last_not_of(" \t") + 1);
        if (!entry.empty()) {
            std::istringstream entry_stream(entry);
            std::string token;
            std::vector<std::string> tokens;
            while (std::getline(entry_stream, token, token_delim)) {
                token.erase(0, token.find_first_not_of(" \t"));
                token.erase(token.find_last_not_of(" \t") + 1);
                if (!token.empty()) {
                    tokens.push_back(token);
                }
            }
            if (!tokens.empty()) {
                entries.push_back(tokens);
            }
        }
    }
    return entries;
}

void SST::Merlin::topo_any::Parse_routing_info(SST::Params &params)
{
    // Read and parse connectivity information from parameter
    // Syntax: "dest router id, port id 1, port id 2; dest router id, ..."
    std::string connectivity_str = params.find<std::string>("connectivity", "");
    if (!connectivity_str.empty())
    {
        auto entries = parse_param_entries(connectivity_str);
        for (const auto& tokens : entries) {
            if (tokens.size() >= 2) {
                try {
                    int dest_router = std::stoi(tokens[0]);
                    std::set<int> port_set;
                    for (size_t i = 1; i < tokens.size(); ++i) {
                        port_set.insert(std::stoi(tokens[i]));
                    }
                    connectivity[dest_router] = port_set;
                }
                catch (const std::exception &e) {
                    output.output("WARNING: Failed to parse connectivity entry: %s\n", tokens[0].c_str());
                }
            }
        }
    }
    else
    {
        fatal(CALL_INFO, -1, "ERROR: the 'connectivity' map is not found in the parameters for anytopo");
    }

    // Read and parse routing table information if dest_tag_routing is used
    // Syntax: "dest router id, next hop id_1, next hop id_2; dest router id, next hop id_1, next hop id_2; ..."
    bool needs_dest_tag_routing_table = false;
    for (int i = 0; i < num_vns; ++i)
    {
        if (vns[i].routing_mode == dest_tag_routing)
        {
            needs_dest_tag_routing_table = true;
            break;
        }
    }
    if (needs_dest_tag_routing_table)
    {
        std::string routing_table_str = params.find<std::string>("routing_table", "");
        if (!routing_table_str.empty())
        {
            auto entries = parse_param_entries(routing_table_str);
            for (const auto& tokens : entries) {
                if (tokens.size() >= 2) {
                    try {
                        int dest_router = std::stoi(tokens[0]);
                        std::vector<int> next_hops;
                        for (size_t i = 1; i < tokens.size(); ++i) {
                            next_hops.push_back(std::stoi(tokens[i]));
                        }
                        Dest_Tag_Routing_Table[dest_router] = next_hops;
                    }
                    catch (const std::exception &e) {
                        output.output("WARNING: Failed to parse routing table entry: %s\n", tokens[0].c_str());
                    }
                }
            }
        }
        else
        {
            output.output("WARNING: dest_tag_routing is enabled but no routing_table parameter provided\n");
        }
    }
}

topo_any::~topo_any() {
    delete[] vns;
    delete rng;
}

void topo_any::setOutputBufferCreditArray(int const* array, int vcs) {
    output_credits      = array;
    assert(vcs==num_vcs);
}

void topo_any::setOutputQueueLengthsArray(int const* array, int vcs) {
    output_queue_lengths = array;
    assert(vcs==num_vcs);
}

Topology::PortState topo_any::getPortState(int port_id) const {
    if (port_id < num_R2R_ports) {
        return R2R;
    } else if (port_id < num_R2R_ports + num_R2N_ports) {
        return R2N;
    } else{
        return UNCONNECTED;
    }
}

int topo_any::getEndpointID(int port_id) {
    if (port_id < num_R2R_ports) {
        assert(!Topology::isHostPort(port_id));
        return -1;
    }else{
        return router_id * num_R2N_ports + (port_id - num_R2R_ports);
    }
}

int topo_any::get_dest_router(int dest_EP_id) const
{
    return dest_EP_id / num_R2N_ports;
}

int topo_any::get_dest_local_port(int dest_EP_id) const
{
    return num_R2R_ports + (dest_EP_id % num_R2N_ports);
}

internal_router_event* topo_any::process_input(RtrEvent* ev) {
    int vn = ev->getRouteVN();
    topo_any_event * returned_ev = new topo_any_event();
    returned_ev->setEncapsulatedEvent(ev);
    returned_ev->setVC(vns[vn].start_vc);
    return returned_ev;
}

void topo_any::route_packet(int port, int vc, internal_router_event* ev) {
    if (vns[vc].routing_mode == source_routing) {
        route_packet_SR(port, vc, static_cast<topo_any_event*>(ev));
    } else {
        route_packet_dest_tag(port, vc, static_cast<topo_any_event*>(ev));
    }
}

void topo_any::route_packet_SR(int port, int vc, topo_any_event* ev) {
    // For source routing, the path should already be in the encapsulated event
    // Update the next router id and port based on the path

    // inspect the path from the encapsulated request, make sure it can be casted to a source routing request
    auto req = ev->inspectRequest();
    if (req == nullptr) {
        fatal(CALL_INFO, -1, "ERROR: Source routing event missing encapsulated request\n");
    }
    
}

//     // Determine VN from VC
//     int vn = 0;
//     int vc_offset = vc;
//     for (int i = 0; i < num_vns; i++) {
//         if (vc_offset < vns[i].num_vcs) {
//             vn = i;
//             break;
//         }
//         vc_offset -= vns[i].num_vcs;
//     }

//     if (vns[vn].routing_mode == source_routing) {
//         // For source routing, the path should already be in the event
//         // Just forward to the next hop
//         if (ev->getNextPort() != -1) {
//             ev->setPort(ev->getNextPort());
//         }
//     } else {
//         // For destination tag routing
//         int dest_router = ev->getDest() / num_R2N_ports; // Assuming endpoints are evenly distributed
        
//         if (dest_router == router_id) {
//             // Destination is local
//             ev->setPort(ev->getDest() % num_R2N_ports);
//         } else {
//             // Route to next hop based on routing table
//             auto it = Dest_Tag_Routing_Table.find(dest_router);
//             if (it != Dest_Tag_Routing_Table.end() && !it->second.empty()) {
//                 int next_hop = it->second[0]; // Simple selection for now
//                 auto conn_it = connectivity.find(next_hop);
//                 if (conn_it != connectivity.end()) {
//                     ev->setPort(conn_it->second);
//                 }
//             }
//         }
//     }
// }


// void topo_any::routeUntimedData(int port, internal_router_event* ev, std::vector<int> &outPorts) {
//     route_packet(port, ev->getVC(), ev);
//     outPorts.push_back(ev->getPort());
// }

// internal_router_event* topo_any::process_UntimedData_input(RtrEvent* ev) {
//     return process_input(ev);
// }
