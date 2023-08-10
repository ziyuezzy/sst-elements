// implemented by Ziyue Zhang Ziyue.Zhang@UGent.be
// This topology class import graph structure and path dictionary from file.

#include <sst_config.h>
#include "from_graph.h"
#include <fstream>
#include <algorithm>
#include <stdlib.h>

using namespace SST::Merlin;

topo_from_graph::topo_from_graph(ComponentId_t cid, Params& params, int num_ports, int rtr_id, int num_vns):
    Topology(cid),
    router_id(rtr_id),
    num_vns(num_vns){

    num_routers = params.find<int>("graph_num_vertices", 1);
    graph_degree = params.find<int>("graph_degree", 1);
    num_local_ports = params.find<int>("hosts_per_router", 1);
    if ( num_ports < (graph_degree+num_local_ports) ) {
        output.fatal(CALL_INFO, -1, "Number of ports per router should be at least %d for this configuration\n", graph_degree+num_local_ports);
    }

    max_path_length = params.find<int>("max_path_length", 0);
    if(!max_path_length)
        output.fatal(CALL_INFO, -1, "Max path length need to be indicated");
    topo_from_graph_event::MAX_PATH_LENGTH = max_path_length;
    local_port_start = graph_degree; //local ports are put at last

    vns = new vn_info[num_vns];
    std::vector<std::string> vn_route_algos;
    if ( params.is_value_array("algorithm") ) {
        params.find_array<std::string>("algorithm", vn_route_algos);
        if ( vn_route_algos.size() != num_vns ) {
            fatal(CALL_INFO, -1, "ERROR: When specifying routing algorithms per VN, algorithm list length must match number of VNs (%d VNs, %lu algorithms).\n",num_vns,vn_route_algos.size());
        }
    }
    else {
        std::string route_algo = params.find<std::string>("algorithm", "nonadaptive");
        for ( int i = 0; i < num_vns; ++i ) vn_route_algos.push_back(route_algo);
    }

    // Setup the routing algorithms
    int curr_vc = 0;
    for ( int i = 0; i < num_vns; ++i ) {
        vns[i].start_vc = curr_vc;
        vns[i].bias = 50;
        if ( !vn_route_algos[i].compare("nonadaptive") ) {
                vns[i].algorithm = nonadaptive_multipath;
                vns[i].num_vcs = max_path_length;
        }
        else if ( !vn_route_algos[i].compare("adaptive") ) {
            vns[i].algorithm = adaptive_multipath;
            vns[i].num_vcs = max_path_length;
        }
        else {
            fatal(CALL_INFO_LONG,1,"ERROR: Unknown routing algorithm specified: %s\n",vn_route_algos[i].c_str());
        }
        curr_vc += vns[i].num_vcs;
    }  
    std::string csv_file_path = params.find<std::string>("csv_files_path", ".");
    std::string RT_path=csv_file_path + "/RT.csv";
    std::string CON_path=csv_file_path + "/CON.csv";

    std::ifstream CON_csv(CON_path);
    std::ifstream RT_csv(RT_path);
    if (!CON_csv) fatal(CALL_INFO_LONG,1,"ERROR opening connectivity csv file: %s", CON_path);
    if (!RT_csv) fatal(CALL_INFO_LONG,1,"ERROR opening connectivity csv file: %s", RT_path);
    // Construct routing tables
    bool in_the_zoon=false;
    std::string line;
    std::getline(RT_csv, line); // skip header line
    while (std::getline(RT_csv, line))
    {
        std::stringstream ss(line);
        std::string sourceStr;
        std::getline(ss, sourceStr, ',');
        int src_id=std::stoi(sourceStr);
        if(src_id==router_id && !in_the_zoon) // enters the correct region for this router
            in_the_zoon=true;
        else if (!in_the_zoon) // still need to enter the correct region for this router
            continue;
        else if (src_id!=router_id && in_the_zoon) // nothing left to read
            break;

        if (in_the_zoon){
            
            std::string destinationStr, ValueStr, nodeStr;
            std::getline(ss, destinationStr, ',');
            std::getline(ss, ValueStr);
            int dest_id = std::stoi(destinationStr);
            std::stringstream path_string(ValueStr);
            std::vector<int> path_to_append;
            path_to_append.clear();
            std::getline(path_string, nodeStr, ' ');
            int next_node = std::stoi(nodeStr);
            assert(next_node==router_id);
            path_to_append.push_back(next_node);
            for (size_t i = 0; i < max_path_length; i++){//TODO: don't need max length anymore
                if(std::getline(path_string, nodeStr, ' ')){
                    next_node = std::stoi(nodeStr);
                    assert(next_node>=0 && next_node<num_routers);
                    path_to_append.push_back(next_node);
                }else{
                    next_node=-1;
                    path_to_append.push_back(next_node);
                }
            }
            routing_table[dest_id].second.push_back(path_to_append);                        
        }
    }
    assert(routing_table.size()==num_routers-1);

    // Construct connectivity map (key is dest id, value is port id)
    in_the_zoon=false;
    line.clear();
    std::getline(CON_csv, line); // skip header line
    while (std::getline(CON_csv, line))
    {
        std::stringstream ss(line);
        std::string sourceStr;
        std::getline(ss, sourceStr, ',');
        int src_id=std::stoi(sourceStr);
        if(src_id==router_id && !in_the_zoon) // enters the correct region for this router
            in_the_zoon=true;
        else if (!in_the_zoon) // still need to enter the correct region for this router
            continue;
        else if (src_id!=router_id && in_the_zoon) // nothing left to read
            break;

        if (in_the_zoon){
            std::string destinationStr, ValueStr;
            std::getline(ss, destinationStr, ',');
            std::getline(ss, ValueStr);
            int dest_id = std::stoi(destinationStr);
            int port_id = std::stoi(ValueStr);
            connectivity[dest_id]=port_id;
        }
    }
    
    for (size_t i = 0; i < num_routers; i++)
        if(i!=router_id)
            routing_table[i].first=0;

}

topo_from_graph::~topo_from_graph(){
    delete[] vns;

    // for (auto& entry : routing_table) {
    //     for (int* path : entry.second.second) {
    //         delete[] path;
    //     }
    //     entry.second.second.clear();
    // } 
    routing_table.clear();

}

void topo_from_graph::route_packet(int port, int vc, internal_router_event* ev){
    int dest_router = get_dest_router(ev->getDest());
    if ( dest_router == router_id ) {
        ev->setNextPort(get_dest_local_port(ev->getDest()));
    } else {
        int vn = ev->getVN();
        if ( vns[vn].algorithm == nonadaptive_multipath ) return route_nonadaptive(port,vc,ev,dest_router);
        else if ( vns[vn].algorithm == adaptive_multipath ) return route_adaptive(port,vc,ev);
        else fatal(CALL_INFO_LONG,1,"ERROR: Unknown routing algorithm encountered");
    }
}

int topo_from_graph::get_dest_router(int dest_id) const
{
    return dest_id / num_local_ports;
}

int topo_from_graph::get_dest_local_port(int dest_id) const
{
    return local_port_start + (dest_id % num_local_ports);
}

void topo_from_graph::route_nonadaptive(int port, int vc, internal_router_event* ev, int dest_router){
    topo_from_graph_event *fg_ev = static_cast<topo_from_graph_event*>(ev);
    if(fg_ev->hops==0){
        assert(fg_ev->path.empty());
        // Determine the path for this packet
        // fg_ev->path=routing_table[dest_router].second[routing_table[dest_router].first];
        for (size_t i = 0; i <= max_path_length; i++)
        {
            fg_ev->path.push_back(routing_table[dest_router].second[routing_table[dest_router].first][i]);
        }
            
        routing_table[dest_router].first=(routing_table[dest_router].first+1)%routing_table[dest_router].second.size(); //update rr counter
    }else{
        assert(!fg_ev->path.empty());
        //nothing to do
    }
    
    fg_ev->setVC(fg_ev->hops);  
    fg_ev->hops++;
    assert(fg_ev->hops <= max_path_length);
    
    // Find the correct port
    int next_router = fg_ev->path[fg_ev->hops];
    assert(next_router!=-1); //the packet should have already reached the destination
    assert(next_router>=0 && next_router<=num_routers && next_router!= router_id && connectivity.count(next_router));
    int p=connectivity[next_router];
    fg_ev->setNextPort(p);    
    
}

void topo_from_graph::route_adaptive(int port, int vc, internal_router_event* ev){
        fatal(CALL_INFO_LONG,1,"ERROR: not yet implemented");
}

internal_router_event* topo_from_graph::process_input(RtrEvent* ev){
    int vn = ev->getRouteVN();
    topo_from_graph_event * fg_ev = new topo_from_graph_event(ev->getDest());
    fg_ev->setEncapsulatedEvent(ev);
    fg_ev->setVC(vns[vn].start_vc);
    return fg_ev;
}

internal_router_event* topo_from_graph::process_UntimedData_input(RtrEvent* ev){
    topo_from_graph_event* fg_ev = new topo_from_graph_event();
    fg_ev->setEncapsulatedEvent(ev);
    fg_ev->dest=ev->getDest(); //TODO: delete 'fg_ev->dest', since the dest can be obtained from ->getDest()?

    return fg_ev;
}

void topo_from_graph::routeUntimedData(int port, internal_router_event* ev, std::vector<int> &outPorts){
    if ( ev->getDest() == UNTIMED_BROADCAST_ADDR ) {
        fatal(CALL_INFO_LONG,1,"ERROR: routeUntimedData for UNTIMED_BROADCAST_ADDR not yet implemented");
        // topo_from_graph_event *fg_ev = static_cast<topo_from_graph_event*>(ev);
        // if (/* condition */)
        // {
        //     /* code */
        // }
    }else{
        route_packet(port, 0, ev);
        outPorts.push_back(ev->getNextPort());
    }
}

Topology::PortState topo_from_graph::getPortState(int port) const
{
    if (port >= local_port_start) {
        if ( port < (local_port_start + num_local_ports) )
            return R2N;
        return UNCONNECTED;
    }
    return R2R;
}

int topo_from_graph::getEndpointID(int port)
{
    if ( !isHostPort(port) ) return -1;
    return (router_id * num_local_ports) + (port - local_port_start);
}


