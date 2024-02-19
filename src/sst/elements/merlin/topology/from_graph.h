// -*- mode: c++ -*-

// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// implemented by Ziyue Zhang Ziyue.Zhang@UGent.be
// This topology class import graph structure and path dictionary from file.

#ifndef COMPONENTS_MERLIN_TOPOLOGY_FROM_GRAPH_H
#define COMPONENTS_MERLIN_TOPOLOGY_FROM_GRAPH_H

#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/params.h>
#include <sst/core/rng/rng.h>

#include "sst/elements/merlin/router.h"


namespace SST {
namespace Merlin {

class topo_from_graph: public Topology{

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        topo_from_graph,
        "merlin",
        "from_graph",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "topology object constructed from graph",
        SST::Merlin::Topology
    )

    SST_ELI_DOCUMENT_PARAMS(
        // Parameters needed for use with old merlin python module
        // {"from_graph.graph_edge_list", "the edge list of a graph that is used to construct the network."},
        // {"from_graph.paths_dict", "the path dictionary of the graph that is used for constructing the routing table."},
        {"from_graph.hosts_per_router", "Number of endpoints attached to each router."},


        // {"graph_edge_list", "the edge list of a graph that is used to construct the network."},
        // {"paths_dict", "the path dictionary of the graph that is used for constructing the routing table."},
        {"hosts_per_router",  "Number of endpoints attached to each router."}
    )

    enum RouteAlgo {
        nonadaptive_multipath,
        adaptive_multipath,
        valiant,
        ugal, //UGAL-L, because UGAL-G is difficult to implement in Merlin (?)
        ugal_precise, //UGAL-L, precisely calculates the valiant path lengths by keeping a distance table in each router
        ugal_threshold, //the cost of the nonminimal path is (2*ql+50), while minimal paths have costs equals to ql
        nonadaptive_weighted
    };

    // A dictionary for routing table
    std::map<int, std::pair<int, std::vector<std::vector<int>>>> routing_table;//key is destination router id, value is a round-robin counter and a list of all possible paths.
    std::map<int, std::vector<std::pair<float, std::vector<int>>>> weighted_routing_table;//key is destination router id, value is a list of weighted paths.
    // A dictionary for port connectivity (which port is connected to which router)
    std::map<int, int> connectivity; //key is destination router id, value is port id. However this assumes that there is no parallel link.
    std::map<std::pair<int, int>, int> distance_table; //key is s-d pair, value is shortest path length (,i.e., distance)

    int router_id; // Router id in the graph
    int num_routers; // number of vertices in the graph
    int graph_degree; //maximal degree in the graph
    int num_local_ports;
    int local_port_start;
    int max_path_length;
    int ugal_val_options;
    // for adaptive routing
    int const* output_credits;
    int const* output_queue_lengths;
    virtual void setOutputBufferCreditArray(int const* array, int vcs);
    virtual void setOutputQueueLengthsArray(int const* array, int vcs);
    int num_vcs;
    RNG::Random* rng;

    int num_vns;

    topo_from_graph(ComponentId_t cid, Params& p, int num_ports, int rtr_id, int num_vns);
    ~topo_from_graph();

    virtual void route_packet(int port, int vc, internal_router_event* ev);
    virtual internal_router_event* process_input(RtrEvent* ev);
    // virtual int routeControlPacket(CtrlRtrEvent* ev); //TODO:??
    virtual void routeUntimedData(int port, internal_router_event* ev, std::vector<int> &outPorts);
    virtual internal_router_event* process_UntimedData_input(RtrEvent* ev);
    
    virtual void getVCsPerVN(std::vector<int>& vcs_per_vn) {
        for ( int i = 0; i < num_vns; ++i ) {
            vcs_per_vn[i] = vns[i].num_vcs;
        }
    }

    virtual PortState getPortState(int port) const;
    virtual int getEndpointID(int port);

    private:
    struct vn_info {
        int start_vc;
        int num_vcs;
        int bias;
        RouteAlgo algorithm;
    };

    vn_info* vns; 
    int get_dest_router(int dest_id) const;
    int get_dest_local_port(int dest_id) const;
    void route_nonadaptive(int port, int vc, internal_router_event* ev, int dest_router);
    void route_nonadaptive_weighted(int port, int vc, internal_router_event* ev, int dest_router);
    void route_valiant(int port, int vc, internal_router_event* ev, int dest_router);
    void route_ugal(int port, int vc, internal_router_event* ev, int dest_router, int num_VAL);
    void route_ugal_precise(int port, int vc, internal_router_event* ev, int dest_router, int num_VAL);
    void route_ugal_threshold(int port, int vc, internal_router_event* ev, int dest_router, int num_VAL);
    void route_adaptive(int port, int vc, internal_router_event* ev, int dest_router);
};


class topo_from_graph_event : public internal_router_event {
public:
    static int MAX_PATH_LENGTH;
    int dest; // destination EP id
    std::vector<int> path; // determined path from the routing table for the packet
    int hops; // keep counts for the number of hops that are already done
    // TODO: QoS (to guarantee on-time pre-fetching)

    int valiant_router; 
    // for valiant routing. -2 means do not do valiant routing; 
    // -1 or '0 or positive number' means do valiant routing:
    // '0 or psitive number' means still in the first segment of valiant path,
    // -1 means the packet is in the last segment of the valiant path.
    int valiant_offset; // offset of the valiant path in the overall path

    topo_from_graph_event() {}
    topo_from_graph_event(int dest) {	
        dest = dest; 
        hops=0; 
        path.clear(); 
        valiant_router=-2; 
        valiant_offset=0;
        }
    virtual ~topo_from_graph_event() { 
        path.clear();
        hops=0;
        }
    virtual internal_router_event* clone(void) override
    {
        return new topo_from_graph_event(*this);
    }

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        internal_router_event::serialize_order(ser);
        ser & dest;
        ser & hops;
        // if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK ) {
        //     path = new int[MAX_PATH_LENGTH];
        // }
        // for ( int i = 0 ; i < MAX_PATH_LENGTH ; i++ ) {
        //     ser & path[i];
        // }
        if (ser.mode() == SST::Core::Serialization::serializer::UNPACK) {
            // If you are deserializing (unpacking), first deserialize the size of the array.
            size_t arraySize;
            ser & arraySize;
            path.resize(arraySize);
        } else {
            // If you are serializing (packing), serialize the size of the array.
            size_t arraySize = path.size();
            ser & arraySize;
        }

        // Now serialize or deserialize the elements of the array.
        for (int i = 0; i < path.size(); i++) {
            ser & path[i];
        }
        // ser & (*path);
    }

protected:

private:
    ImplementSerializable(SST::Merlin::topo_from_graph_event)

};
int topo_from_graph_event::MAX_PATH_LENGTH; 


}
}


#endif