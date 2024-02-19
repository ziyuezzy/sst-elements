// implemented by Ziyue Zhang Ziyue.Zhang@UGent.be
// This topology class import graph structure and path dictionary from file.

#include <sst_config.h>
#include "from_graph.h"
#include <fstream>
#include <algorithm>
#include <stdlib.h>
#include "sst/core/rng/xorshift.h"

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

    ugal_val_options = params.find<int>("ugal_val_options", 4);
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

    bool construct_distance_table = false;
    bool path_with_weights =false;

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
        else if ( !vn_route_algos[i].compare("valiant") ) {
            vns[i].algorithm = valiant;
            vns[i].num_vcs = max_path_length*2;
        }
        else if ( !vn_route_algos[i].compare("ugal") ) {
            vns[i].algorithm = ugal;
            vns[i].num_vcs = max_path_length*2;
        }
        else if ( !vn_route_algos[i].compare("ugal_threshold") ) {
            vns[i].algorithm = ugal_threshold;
            vns[i].num_vcs = max_path_length*2;
        }
        else if ( !vn_route_algos[i].compare("ugal_precise") ) {
            vns[i].algorithm = ugal_precise;
            construct_distance_table=true;
            vns[i].num_vcs = max_path_length*2;
        }
        else if ( !vn_route_algos[i].compare("nonadaptive_weighted") ) {
            vns[i].algorithm = nonadaptive_weighted;
            path_with_weights = true;
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
        if (!std::isdigit(sourceStr[0])){
            // should be the last line
            assert(!std::getline(RT_csv, line));
            break;
        }
        
        int src_id=std::stoi(sourceStr);
        if(src_id==router_id && !in_the_zoon){ // enters the correct region for this router
            in_the_zoon=true;
        }else if (!in_the_zoon){ // still need to enter the correct region for this router
            if (!construct_distance_table){
                continue;
            }
        }else if (src_id!=router_id && in_the_zoon){ // nothing left to read, leave the zoon
            in_the_zoon=false;
            if (!construct_distance_table){
                break;
            }
        }// else, it is in the zoon

        if(!path_with_weights){
            std::string destinationStr, ValueStr, nodeStr;
            std::getline(ss, destinationStr, ',');
            std::getline(ss, ValueStr);
            int dest_id = std::stoi(destinationStr);

            if (construct_distance_table && distance_table.count(std::make_pair(src_id, dest_id))){
                continue; //This assumes that the first encountered path is the shortest path
            }


            std::stringstream path_string(ValueStr);
            std::vector<int> path_to_append;
            path_to_append.clear();
            std::getline(path_string, nodeStr, ' ');
            int next_node = std::stoi(nodeStr);
            if(in_the_zoon) assert(next_node==router_id);
            path_to_append.push_back(next_node);
            for (size_t i = 0; i < max_path_length; i++){
                if(std::getline(path_string, nodeStr, ' ')){
                    next_node = std::stoi(nodeStr);
                    assert(next_node>=0 && next_node<num_routers);
                    path_to_append.push_back(next_node);
                }else{
                    // next_node=-1;
                    // path_to_append.push_back(next_node);
                }
            }
            if (in_the_zoon){
                routing_table[dest_id].second.push_back(path_to_append);  
            }else if (construct_distance_table)
            {
                distance_table[std::make_pair(src_id, dest_id)]=path_to_append.size()-1;
            }    
        }else{
            std::string destinationStr, weightStr, ValueStr, nodeStr;
            std::getline(ss, destinationStr, ',');
            std::getline(ss, weightStr, ',');
            std::getline(ss, ValueStr);
            int dest_id = std::stoi(destinationStr);
            float path_weight = std::stof(weightStr);

            if (construct_distance_table && distance_table.count(std::make_pair(src_id, dest_id))){
                continue; //This assumes that the first encountered path is the shortest path
            }


            std::stringstream path_string(ValueStr);
            std::vector<int> path_to_append;
            path_to_append.clear();
            std::getline(path_string, nodeStr, ' ');
            int next_node = std::stoi(nodeStr);
            if(in_the_zoon) assert(next_node==router_id);
            path_to_append.push_back(next_node);
            for (size_t i = 0; i < max_path_length; i++){
                if(std::getline(path_string, nodeStr, ' ')){
                    next_node = std::stoi(nodeStr);
                    assert(next_node>=0 && next_node<num_routers);
                    path_to_append.push_back(next_node);
                }else{
                    // next_node=-1;
                    // path_to_append.push_back(next_node);
                }
            }
            if (in_the_zoon){
                weighted_routing_table[dest_id].push_back(std::make_pair(path_weight, path_to_append));  
            }else if (construct_distance_table)
            {
                distance_table[std::make_pair(src_id, dest_id)]=path_to_append.size()-1;
            }    
        }          
        
    }
    if(!path_with_weights)
        assert(routing_table.size()==num_routers-1);
    else
        assert(weighted_routing_table.size()==num_routers-1);


    if (construct_distance_table)
        assert(distance_table.size()==(num_routers-1)*(num_routers-1));

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

    rng = new RNG::XORShiftRNG(rtr_id+1);
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
    connectivity.clear();
    distance_table.clear();

}

void topo_from_graph::route_packet(int port, int vc, internal_router_event* ev){
    int dest_router = get_dest_router(ev->getDest());
    if ( dest_router == router_id ) {
        ev->setNextPort(get_dest_local_port(ev->getDest()));
    } else {
        int vn = ev->getVN();
        if ( vns[vn].algorithm == nonadaptive_multipath ) return route_nonadaptive(port,vc,ev,dest_router);
        else if ( vns[vn].algorithm == adaptive_multipath ) return route_adaptive(port,vc,ev,dest_router);
        else if ( vns[vn].algorithm == valiant ) return route_valiant(port,vc,ev,dest_router);
        else if ( vns[vn].algorithm == ugal ) return route_ugal(port,vc,ev,dest_router, ugal_val_options);
        else if ( vns[vn].algorithm == ugal_precise ) return route_ugal_precise(port,vc,ev,dest_router, ugal_val_options);
        else if ( vns[vn].algorithm == ugal_threshold ) return route_ugal_threshold(port,vc,ev,dest_router, ugal_val_options);
        else if ( vns[vn].algorithm == nonadaptive_weighted ) return route_nonadaptive_weighted(port,vc,ev,dest_router);
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
        std::vector<int> temp_path=routing_table[dest_router].second[routing_table[dest_router].first];
        for (auto i: temp_path) fg_ev->path.push_back(i);
            
        routing_table[dest_router].first=(routing_table[dest_router].first+1)%routing_table[dest_router].second.size(); //update rr counter
    }else{
        assert(!fg_ev->path.empty());
        //nothing to do
    }
    
    fg_ev->setVC(fg_ev->hops);  
    fg_ev->hops++;
    assert(fg_ev->hops <= max_path_length);
    
    // Find the correct port
    assert( fg_ev->path.size() > fg_ev->hops && "the packet should have already reached the destination");
    int next_router = fg_ev->path[fg_ev->hops];
    
    assert(next_router>=0 && next_router<=num_routers && next_router!= router_id && connectivity.count(next_router));
    int p=connectivity[next_router];
    fg_ev->setNextPort(p);    
    
}

void topo_from_graph::route_nonadaptive_weighted(int port, int vc, internal_router_event* ev, int dest_router){
    
    topo_from_graph_event *fg_ev = static_cast<topo_from_graph_event*>(ev);
    if(fg_ev->hops==0){
        assert(fg_ev->path.empty());
        // Determine the path for this packet   
        float dice=rng->nextUniform();
        float _sum=0.0;
        for (auto path : weighted_routing_table[dest_router])
        {
            _sum+=path.first;
            if(dice>_sum){
                continue;
            }else{
                for (auto i: path.second) fg_ev->path.push_back(i);
                break;
            }
        }
            
    }
    assert(!fg_ev->path.empty());
    
    fg_ev->setVC(fg_ev->hops);  
    fg_ev->hops++;
    assert(fg_ev->hops <= max_path_length);
    
    // Find the correct port
    assert( fg_ev->path.size() > fg_ev->hops && "the packet should have already reached the destination");
    int next_router = fg_ev->path[fg_ev->hops];
    
    assert(next_router>=0 && next_router<=num_routers && next_router!= router_id && connectivity.count(next_router));
    int p=connectivity[next_router];
    fg_ev->setNextPort(p);    
    
}



// void topo_from_graph::route_adaptive(int port, int vc, internal_router_event* ev, int dest_router){
// // (for now, do not support the path to exceed 'max_path_length' in the imported routing table, but //TODO: this may be able to change)
//     // weight all paths (that do not exceeds max_path_length) in the routing table
//     // weight = local queue lengths at the corresponding vc
//     int next_vc=vc+1;
//     assert(next_vc<num_vcs);
//     int min_weight = std::numeric_limits<int>::max();
//     std::vector<std::pair<int,std::vector<int>> > min_routes;  //pair < port id, path vector >
//     topo_from_graph_event *fg_ev = static_cast<topo_from_graph_event*>(ev);
//     for (std::vector<int> path : routing_table[dest_router].second){
//         if(fg_ev->hops + path.size()-1 > max_path_length){// if the accumulated length exceeds max_path_length, ignore
//             continue;
//         }else{//otherwise, calculate weight
//             int next_router=path[1];
//             int next_port=connectivity[next_router];
//             int weight = output_queue_lengths[next_port * num_vcs + next_vc];  //TODO: need to consider two cases: hop=0 or not
//             if ( weight == min_weight){
//                 min_routes.push_back(std::make_pair(next_port, path));
//             }else if(weight < min_weight){
//                 min_weight=weight;
//                 min_routes.clear();
//                 min_routes.push_back(std::make_pair(next_port, path));
//             }
//         }
//     }
//     assert(!min_routes.empty());
//     std::pair<int,std::vector<int>> & route = min_routes[rng->generateNextUInt32() % min_routes.size()];
//     fg_ev->setNextPort(route.first);

//     fg_ev->path.clear();
//     for (auto i: route.second) fg_ev->path.push_back(i); // Just directly replace the path here. If it is not good?

//     fg_ev->setVC(fg_ev->hops);  
//     fg_ev->hops++;
//     assert(fg_ev->hops <= max_path_length);
        
//         // fatal(CALL_INFO_LONG,1,"ERROR: not yet implemented");
// }

void topo_from_graph::route_valiant(int port, int vc, internal_router_event* ev, int dest_router){
    topo_from_graph_event *fg_ev = static_cast<topo_from_graph_event*>(ev);

    // set valiant indicator
    if (fg_ev->valiant_router==-2){
        assert(fg_ev->path.empty());
        // if indicator is -2, set random intermidiate router
        int intermidiate_router=rng->generateNextUInt64()%num_routers;  
        while (intermidiate_router==router_id)
            intermidiate_router=rng->generateNextUInt64()%num_routers;        
        fg_ev->valiant_router = intermidiate_router;

        std::vector<int> temp_path=routing_table[intermidiate_router].second[routing_table[intermidiate_router].first];
        for (auto i: temp_path) fg_ev->path.push_back(i); 
        routing_table[intermidiate_router].first=(routing_table[intermidiate_router].first+1)%routing_table[intermidiate_router].second.size(); //update rr counter    
    }
    
    if (fg_ev->valiant_router>=0)
    {
        if(fg_ev->valiant_router==router_id){
            // if arriving at the intermidiate routing, and indicator is 0 or positive, set it to -1
            fg_ev->valiant_router=-1;

            // TODO: checks if the second segment of valiant path has overlap with the first segment??
            // // for (size_t i = 0; i < routing_table[dest_router].second.size(); i++)
            // // {
            // //     bool overlap = false;
            // //     std::vector<int> temp_path=routing_table[dest_router].second[(routing_table[dest_router].first+i)%routing_table[dest_router].second.size()];
            // //     for (auto u: temp_path){
            // //         for (auto v: fg_ev->path){
            // //             if (u==v){
            // //                 overlap = true;
            // //                 break;
            // //             }
            // //         }
            // //     }
            // // }
            
            // append path for the next segment, set path offset
            std::vector<int> temp_path=routing_table[dest_router].second[routing_table[dest_router].first];
            for (size_t i = 1; i < temp_path.size(); i++)
                fg_ev->path.push_back(temp_path[i]); // Avoid doubling the id of intermidiate router
            // for (auto i: temp_path) fg_ev->path.push_back(i); 
            routing_table[dest_router].first=(routing_table[dest_router].first+1)%routing_table[dest_router].second.size(); //update rr counter    
            fg_ev->valiant_offset=fg_ev->hops;

        }else{
            // else forward packet
            fg_ev->setVC(fg_ev->hops);  
            fg_ev->hops++;
            assert(fg_ev->hops <= max_path_length);
            // Find the correct port
            assert( fg_ev->path.size() > fg_ev->hops && "the packet should have already reached the destination");
            int next_router = fg_ev->path[fg_ev->hops];
            assert(next_router>=0 && next_router<=num_routers && next_router!= router_id && connectivity.count(next_router));
            int p=connectivity[next_router];
            fg_ev->setNextPort(p);   
        }   

    }
    
    if(fg_ev->valiant_router==-1){
        //indicator is -1, meaning the packet has already been routed valiantly
        fg_ev->setVC(fg_ev->hops);  
        // fg_ev->setVC(fg_ev->hops - fg_ev->valiant_offset);  
        fg_ev->hops++;
        assert(fg_ev->hops <= 2*max_path_length);
        // Find the correct port
        assert( fg_ev->path.size() > fg_ev->hops && "the packet should have already reached the destination");
        int next_router = fg_ev->path[fg_ev->hops];
        assert(next_router>=0 && next_router<=num_routers && next_router!= router_id && connectivity.count(next_router));
        int p=connectivity[next_router];
        fg_ev->setNextPort(p);   
    }
}

void topo_from_graph::route_ugal(int port, int vc, internal_router_event* ev, int dest_router, int num_VAL){
    topo_from_graph_event *fg_ev = static_cast<topo_from_graph_event*>(ev);

    if (fg_ev->hops==0){
        // need to decide whether or not use valiant route
        std::map< std::vector<int>, int > possible_paths; // keys are paths, values are costs
        // generate num_VAL valiant paths, calculate costs
        // However, we do not know yet the total path length, but only the first segment of valiant path.
        while (possible_paths.size() < num_VAL)
        {
            int intermidiate_router=rng->generateNextUInt64()%num_routers;
            if (intermidiate_router == router_id || intermidiate_router == dest_router) continue;
            
            unsigned int random_number = rng->generateNextUInt32()%routing_table[intermidiate_router].second.size();
            std::vector<int> temp_path=routing_table[intermidiate_router].second[random_number];
            if (possible_paths.count(temp_path)) continue;

            int next_router=temp_path[1];
            int next_port=connectivity[next_router];
            assert(vc==0);
            int queue_length = output_queue_lengths[next_port * num_vcs];
            // int queue_length = output_queue_lengths[next_port * num_vcs + vc];
            // assume the second valiant path has max_path_length, in order to calculate the cost
            // possible_paths[temp_path]= queue_length;
            possible_paths[temp_path]= ((temp_path.size()-1) + max_path_length)*queue_length;
        }
        
        // add shortest paths and calculate costs
        for (std::vector<int> path : routing_table[dest_router].second){
            int next_router=path[1];
            int next_port=connectivity[next_router];
            assert(vc==0);
            // possible_paths[path] = (output_queue_lengths[next_port * num_vcs]);
            possible_paths[path] = (path.size()-1)*output_queue_lengths[next_port * num_vcs];
        }
        // choose the least-cost path
        int min_weight = std::numeric_limits<int>::max();
        std::vector<std::pair<int,std::vector<int>> > min_routes;  //pair < port id, path vector >
        for(auto p = possible_paths.begin(); p != possible_paths.end(); p++){
            int weight = p->second;
            int next_router=p->first[1];
            int next_port=connectivity[next_router];
            if ( weight == min_weight){
                min_routes.push_back(std::make_pair(next_port, p->first));
            }else if(weight < min_weight){
                min_weight=weight;
                min_routes.clear();
                min_routes.push_back(std::make_pair(next_port, p->first));
            }
        }
        assert(!min_routes.empty());
        std::pair<int,std::vector<int>> & route = min_routes[rng->generateNextUInt32() % min_routes.size()]; 
        fg_ev->setNextPort(route.first);
        for (auto i: route.second) fg_ev->path.push_back(i);
        if(route.second.back() != dest_router){
            fg_ev->valiant_router=route.second.back();
            // if(router_id==6){
            //     printf("valiant dest %d (via port %d) is chosen for source %d dest %d \n",route.second.back(), route.first, router_id, dest_router);
            // }
        }else{
            fg_ev->valiant_router=-2;
            // if(router_id==6){
            //     printf("direct path is chosen for source %d dest %d via port %d \n", router_id, dest_router, route.first);
            // }
        }
        fg_ev->setVC(fg_ev->hops);  
        fg_ev->hops++;
        // assert(fg_ev->hops <= max_path_length);
        
    }else{ // if not the first hop
        if (fg_ev->valiant_router >= 0){
            // if in the first segment of valiant path
            if(router_id==fg_ev->valiant_router){
                // if it has reached the intermidiate router
                fg_ev->valiant_router=-1;

                // append path for the next segment, set path offset
                std::vector<int> temp_path=routing_table[dest_router].second[routing_table[dest_router].first];

                // for (auto i: temp_path) fg_ev->path.push_back(i); 
                for (size_t i = 1; i < temp_path.size(); i++)
                    fg_ev->path.push_back(temp_path[i]); // Avoid doubling the id of intermidiate router

                routing_table[dest_router].first=(routing_table[dest_router].first+1)%routing_table[dest_router].second.size(); //update rr counter    //TODO: random?
                fg_ev->valiant_offset=fg_ev->hops;
            }else{
                // else forward packet
                fg_ev->setVC(fg_ev->hops);  
                fg_ev->hops++;
                assert(fg_ev->hops <= max_path_length);
                // Find the correct port
                assert( fg_ev->path.size() > fg_ev->hops && "the packet should have already reached the destination");
                int next_router = fg_ev->path[fg_ev->hops];
                assert(next_router>=0 && next_router<=num_routers && next_router!= router_id && connectivity.count(next_router));
                int p=connectivity[next_router];
                fg_ev->setNextPort(p);   
            }
        }

        if(fg_ev->valiant_router==-1){
                // if in the second segment of valiant path
                fg_ev->setVC(fg_ev->hops);  
                // fg_ev->setVC(fg_ev->hops - fg_ev->valiant_offset);  
                fg_ev->hops++;
                assert(fg_ev->hops <= 2*max_path_length);
                // Find the correct port
                assert( fg_ev->path.size() > fg_ev->hops && "the packet should have already reached the destination");
                int next_router = fg_ev->path[fg_ev->hops];
                assert(next_router>=0 && next_router<=num_routers && next_router!= router_id && connectivity.count(next_router));
                int p=connectivity[next_router];
                fg_ev->setNextPort(p);   
        }
        
        if(fg_ev->valiant_router==-2){
            // if it is routed minimally
            // else forward packet
            fg_ev->setVC(fg_ev->hops);  
            fg_ev->hops++;
            assert(fg_ev->hops <= max_path_length);
            // Find the correct port
            assert( fg_ev->path.size() > fg_ev->hops && "the packet should have already reached the destination");
            int next_router = fg_ev->path[fg_ev->hops];
            assert(next_router>=0 && next_router<=num_routers && next_router!= router_id && connectivity.count(next_router));
            int p=connectivity[next_router];
            fg_ev->setNextPort(p);   
        }
    }

}
void topo_from_graph::route_ugal_precise(int port, int vc, internal_router_event* ev, int dest_router, int num_VAL){
    topo_from_graph_event *fg_ev = static_cast<topo_from_graph_event*>(ev);

    if (fg_ev->hops==0){
        // need to decide whether or not use valiant route
        std::map< std::vector<int>, int > possible_paths; // keys are paths, values are costs
        // generate num_VAL valiant paths, calculate costs
        // However, we do not know yet the total path length, but only the first segment of valiant path.
        while (possible_paths.size() < num_VAL)
        {
            int intermidiate_router=rng->generateNextUInt64()%num_routers;
            if (intermidiate_router == router_id || intermidiate_router == dest_router) continue;
            
            unsigned int random_number = rng->generateNextUInt32()%routing_table[intermidiate_router].second.size();
            std::vector<int> temp_path=routing_table[intermidiate_router].second[random_number];
            if (possible_paths.count(temp_path)) continue;

            int next_router=temp_path[1];
            int next_port=connectivity[next_router];
            assert(vc==0);
            int queue_length = output_queue_lengths[next_port * num_vcs];
            int path_length=(temp_path.size()-1)+distance_table[std::make_pair(intermidiate_router, dest_router)];
            possible_paths[temp_path]=path_length*queue_length+1;//the +1 is to break tie against shortest path
        }
        
        // add shortest paths and calculate costs
        for (std::vector<int> path : routing_table[dest_router].second){
            int next_router=path[1];
            int next_port=connectivity[next_router];
            assert(vc==0);
            possible_paths[path] = (path.size()-1)*output_queue_lengths[next_port * num_vcs];
        }
        // choose the least-cost path
        int min_weight = std::numeric_limits<int>::max();
        std::vector<std::pair<int,std::vector<int>> > min_routes;  //pair < port id, path vector >
        for(auto p = possible_paths.begin(); p != possible_paths.end(); p++){
            int weight = p->second;
            int next_router=p->first[1];
            int next_port=connectivity[next_router];
            if ( weight == min_weight){
                min_routes.push_back(std::make_pair(next_port, p->first));
            }else if(weight < min_weight){
                min_weight=weight;
                min_routes.clear();
                min_routes.push_back(std::make_pair(next_port, p->first));
            }
        }
        assert(!min_routes.empty());
        std::pair<int,std::vector<int>> & route = min_routes[rng->generateNextUInt32() % min_routes.size()]; 
        fg_ev->setNextPort(route.first);
        for (auto i: route.second) fg_ev->path.push_back(i);
        if(route.second.back() != dest_router){
            fg_ev->valiant_router=route.second.back();
            // if(router_id==6){
            //     printf("valiant dest %d (via port %d) is chosen for source %d dest %d \n",route.second.back(), route.first, router_id, dest_router);
            // }
        }else{
            fg_ev->valiant_router=-2;
            // if(router_id==6){
            //     printf("direct path is chosen for source %d dest %d via port %d \n", router_id, dest_router, route.first);
            // }
        }
        fg_ev->setVC(fg_ev->hops);  
        fg_ev->hops++;
        // assert(fg_ev->hops <= max_path_length);
        
    }else{ // if not the first hop (assume all shortest path for now //TODO: ?)
        if (fg_ev->valiant_router >= 0){
            // if in the first segment of valiant path
            if(router_id==fg_ev->valiant_router){
                // if it has reached the intermidiate router
                fg_ev->valiant_router=-1;

                // append path for the next segment, set path offset
                std::vector<int> temp_path=routing_table[dest_router].second[routing_table[dest_router].first];

                // for (auto i: temp_path) fg_ev->path.push_back(i); 
                for (size_t i = 1; i < temp_path.size(); i++)
                    fg_ev->path.push_back(temp_path[i]); // Avoid doubling the id of intermidiate router

                routing_table[dest_router].first=(routing_table[dest_router].first+1)%routing_table[dest_router].second.size(); //update rr counter    //TODO: random?
                fg_ev->valiant_offset=fg_ev->hops;
            }else{
                // else forward packet
                fg_ev->setVC(fg_ev->hops);  
                fg_ev->hops++;
                assert(fg_ev->hops <= max_path_length);
                // Find the correct port
                assert( fg_ev->path.size() > fg_ev->hops && "the packet should have already reached the destination");
                int next_router = fg_ev->path[fg_ev->hops];
                assert(next_router>=0 && next_router<=num_routers && next_router!= router_id && connectivity.count(next_router));
                int p=connectivity[next_router];
                fg_ev->setNextPort(p);   
            }
        }

        if(fg_ev->valiant_router==-1){
                // if in the second segment of valiant path
                fg_ev->setVC(fg_ev->hops);  
                // fg_ev->setVC(fg_ev->hops - fg_ev->valiant_offset);  
                fg_ev->hops++;
                assert(fg_ev->hops <= 2*max_path_length);
                // Find the correct port
                assert( fg_ev->path.size() > fg_ev->hops && "the packet should have already reached the destination");
                int next_router = fg_ev->path[fg_ev->hops];
                assert(next_router>=0 && next_router<=num_routers && next_router!= router_id && connectivity.count(next_router));
                int p=connectivity[next_router];
                fg_ev->setNextPort(p);   
        }
        
        if(fg_ev->valiant_router==-2){
            // if it is routed minimally
            // else forward packet
            fg_ev->setVC(fg_ev->hops);  
            fg_ev->hops++;
            assert(fg_ev->hops <= max_path_length);
            // Find the correct port
            assert( fg_ev->path.size() > fg_ev->hops && "the packet should have already reached the destination");
            int next_router = fg_ev->path[fg_ev->hops];
            assert(next_router>=0 && next_router<=num_routers && next_router!= router_id && connectivity.count(next_router));
            int p=connectivity[next_router];
            fg_ev->setNextPort(p);   
        }
    }

}

void topo_from_graph::route_ugal_threshold(int port, int vc, internal_router_event* ev, int dest_router, int num_VAL){
    topo_from_graph_event *fg_ev = static_cast<topo_from_graph_event*>(ev);
    // int next_vc=vc+1;
    // assert(next_vc<num_vcs);

    if (fg_ev->hops==0){
        // need to decide whether or not use valiant route
        std::map< std::vector<int>, int > possible_paths; // keys are paths, values are costs
        // generate num_VAL valiant paths, calculate costs
        // However, we do not know yet the total path length, but only the first segment of valiant path.
        while (possible_paths.size() < num_VAL)
        {
            int intermidiate_router=rng->generateNextUInt64()%num_routers;
            if (intermidiate_router == router_id || intermidiate_router == dest_router) continue;
            
            unsigned int random_number = rng->generateNextUInt32()%routing_table[intermidiate_router].second.size();
            std::vector<int> temp_path=routing_table[intermidiate_router].second[random_number];

            int next_router=temp_path[1];
            int next_port=connectivity[next_router];
            int queue_length = output_queue_lengths[next_port * num_vcs + vc];
            // assume the second valiant path has max_path_length, in order to calculate the cost
            possible_paths[temp_path]= 2*queue_length + 50; // the '+1' is for breaking draw with minimal paths, therefore minimal paths are prior to be chosen
        }
        
        // add shortest paths and calculate costs
        for (std::vector<int> path : routing_table[dest_router].second){
            int next_router=path[1];
            int next_port=connectivity[next_router];
            possible_paths[path] = output_queue_lengths[next_port * num_vcs + vc];
        }
        // choose the least-cost path
        int min_weight = std::numeric_limits<int>::max();
        std::vector<std::pair<int,std::vector<int>> > min_routes;  //pair < port id, path vector >
        for(auto p = possible_paths.begin(); p != possible_paths.end(); p++){
            int weight = p->second;
            int next_router=p->first[1];
            int next_port=connectivity[next_router];
            if ( weight == min_weight){
                min_routes.push_back(std::make_pair(next_port, p->first));
            }else if(weight < min_weight){
                min_weight=weight;
                min_routes.clear();
                min_routes.push_back(std::make_pair(next_port, p->first));
            }
        }
        assert(!min_routes.empty());
        std::pair<int,std::vector<int>> & route = min_routes[rng->generateNextUInt32() % min_routes.size()]; 
        fg_ev->setNextPort(route.first);
        for (auto i: route.second) fg_ev->path.push_back(i);
        if(route.second.back() != dest_router)
            fg_ev->valiant_router=route.second.back();
        else
            fg_ev->valiant_router=-2;
        fg_ev->setVC(fg_ev->hops);  
        fg_ev->hops++;
        // assert(fg_ev->hops <= max_path_length);
        
    }else{ // if not the first hop
        if (fg_ev->valiant_router >= 0){
            // if in the first segment of valiant path
            if(router_id==fg_ev->valiant_router){
                // if it has reached the intermidiate router
                fg_ev->valiant_router=-1;

                // append path for the next segment, set path offset
                std::vector<int> temp_path=routing_table[dest_router].second[routing_table[dest_router].first];

                // for (auto i: temp_path) fg_ev->path.push_back(i); 
                for (size_t i = 1; i < temp_path.size(); i++)
                    fg_ev->path.push_back(temp_path[i]); // Avoid doubling the id of intermidiate router

                routing_table[dest_router].first=(routing_table[dest_router].first+1)%routing_table[dest_router].second.size(); //update rr counter    //TODO: random?
                fg_ev->valiant_offset=fg_ev->hops;
            }else{
                // else forward packet
                fg_ev->setVC(fg_ev->hops);  
                fg_ev->hops++;
                assert(fg_ev->hops <= max_path_length);
                // Find the correct port
                assert( fg_ev->path.size() > fg_ev->hops && "the packet should have already reached the destination");
                int next_router = fg_ev->path[fg_ev->hops];
                assert(next_router>=0 && next_router<=num_routers && next_router!= router_id && connectivity.count(next_router));
                int p=connectivity[next_router];
                fg_ev->setNextPort(p);   
            }
        }

        if(fg_ev->valiant_router==-1){
                // if in the second segment of valiant path
                fg_ev->setVC(fg_ev->hops);  
                // fg_ev->setVC(fg_ev->hops - fg_ev->valiant_offset);  
                fg_ev->hops++;
                assert(fg_ev->hops <= 2*max_path_length);
                // Find the correct port
                assert( fg_ev->path.size() > fg_ev->hops && "the packet should have already reached the destination");
                int next_router = fg_ev->path[fg_ev->hops];
                assert(next_router>=0 && next_router<=num_routers && next_router!= router_id && connectivity.count(next_router));
                int p=connectivity[next_router];
                fg_ev->setNextPort(p);   
        }
        
        if(fg_ev->valiant_router==-2){
            // if it is routed minimally
            // else forward packet
            fg_ev->setVC(fg_ev->hops);  
            fg_ev->hops++;
            assert(fg_ev->hops <= max_path_length);
            // Find the correct port
            assert( fg_ev->path.size() > fg_ev->hops && "the packet should have already reached the destination");
            int next_router = fg_ev->path[fg_ev->hops];
            assert(next_router>=0 && next_router<=num_routers && next_router!= router_id && connectivity.count(next_router));
            int p=connectivity[next_router];
            fg_ev->setNextPort(p);   
        }
    }

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
    // fg_ev->dest=ev->getDest();

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
            
        int dest_router = get_dest_router(ev->getDest());
        if ( dest_router == router_id ) {
            ev->setNextPort(get_dest_local_port(ev->getDest()));
        } else {
            int vn = ev->getVN();
            if ( vns[vn].algorithm == nonadaptive_weighted )  //TODO: if add more weighted routing algo, this need to be appended
                route_nonadaptive_weighted(port,0,ev,dest_router);
            else
                route_nonadaptive(port,0,ev,dest_router);
            
        }
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

void
topo_from_graph::setOutputBufferCreditArray(int const* array, int vcs)
{
    output_credits = array;
    num_vcs = vcs;
}

void
topo_from_graph::setOutputQueueLengthsArray(int const* array, int vcs)
{
    output_queue_lengths = array;
    num_vcs = vcs;
}
