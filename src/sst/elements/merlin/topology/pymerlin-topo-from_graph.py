#!/usr/bin/env python
#
# implemented by Ziyue Zhang Ziyue.Zhang@UGent.be
# This topology class import graph structure and path dictionary from file.

import sst
from sst.merlin.base import *
import pickle
import csv

class topoFromGraph(Topology):

    def __init__(self):
        Topology.__init__(self)
        self._declareClassVariables(["link_latency", "host_link_latency", "topo_name", "edgelist_file", "pathdict_file"])
        self._declareParams("main",["hosts_per_router","graph_num_vertices","graph_degree",
                                    "max_path_length","algorithm","adaptive_threshold", 'csv_files_path'])
        self._subscribeToPlatformParamSet("topology")
    
    def getName(self):
        return self.topo_name
    
    def getNumNodes(self):
        return self.graph_num_vertices*self.hosts_per_router
    
    def getRouterNameForId(self,rtr_id):
        return "%srouter%d"%(self._prefix, rtr_id)
    
    def findRouterByLocation(self,rtr_id):
        return sst.getRouterNameForId(self.getRouterNameForId(rtr_id))
    
    def build(self, endpoint):
        #define routers
        routers=[None]*self.graph_num_vertices
        router_radix=self.hosts_per_router+self.graph_degree
        for r in range(self.graph_num_vertices):
            routers[r] = self._instanceRouter(router_radix,r)
            # topo = routers[r].setSubComponent(self.router.getTopologySlotName(),"merlin.from_graph",0)
            # self._applyStatisticsSettings(topo)
            # topo.addParams(self._getGroupParams("main"))
            
            #connect endpoints to routers
            port_id=self.graph_degree
            for n in range(self.hosts_per_router):
                nodeID = self.hosts_per_router * r + n
                (ep, port_name) = endpoint.build(nodeID, {})
                if ep:
                    nicLink = sst.Link("nic_%d_%d"%(r, n))
                    # if self.bundleEndpoints:
                    #     nicLink.setNoCut()
                    nicLink.connect( (ep, port_name, self.host_link_latency), (routers[r], "port%d"%port_id, self.host_link_latency) )
                    # print(f"router {r}'s port{port_id} is connected to EP {nodeID}'s port {port_name}")
                    port_id+=1
            assert(port_id==router_radix)
        
        #read picked graph structure from file
        _CON=[]
        edgelist=pickle.load(open(self.edgelist_file, 'rb'))
        edgelist.sort() # sort the edge list, for the csv file to be more clean
        router_next_port=[0]*self.graph_num_vertices
        #interconnect routers
        for edge in edgelist:
            source=edge[0]
            dest=edge[1]
            _CON.append([source, dest, router_next_port[source]])
            _CON.append([dest, source, router_next_port[dest]])
            link = sst.Link("link_%d_%d"%(source, dest))
            routers[source].addLink(link, "port%d"%router_next_port[source], self.link_latency)
            router_next_port[source]+=1
            routers[dest].addLink(link, "port%d"%router_next_port[dest], self.link_latency)
            router_next_port[dest]+=1
        _CON.sort()
        assert(max(router_next_port)==self.graph_degree)
        assert(min(router_next_port)==self.graph_degree) #If it is a regular graph, otherwise not implemented
        #TODO: if the degree of a certain vertex is not exactly the maximal? (GDBG and polarfly)
        del edgelist
        #write csv file to store connectivity 
        with open(f'{self.csv_files_path}/CON.csv', mode='w') as csv_CON:
            csv_writer = csv.writer(csv_CON, delimiter=',', quotechar='"', quoting=csv.QUOTE_MINIMAL)
            csv_writer.writerow(["source", "dest", "port"])
            for row in _CON:
                csv_writer.writerow([row[0], row[1], row[2]])

        #write csv file to store routing table 
        self.max_path_length=0
        with open(f'{self.csv_files_path}/RT.csv', mode='w') as csv_RT:
            csv_writer = csv.writer(csv_RT, delimiter=',', quotechar='"', quoting=csv.QUOTE_MINIMAL)
            csv_writer.writerow(["source", "dest", "path"])

            pathdict=dict(sorted(pickle.load(open(self.pathdict_file, 'rb')).items()))
            for (source, destination), paths in pathdict.items():
                for path in paths:
                    csv_writer.writerow([source, destination, ' '.join(str(v) for v in path)])
                    if len(path)-1 > self.max_path_length:
                        self.max_path_length = len(path)-1

        
        for r in range(self.graph_num_vertices):
            topo = routers[r].setSubComponent(self.router.getTopologySlotName(),"merlin.from_graph",0)
            self._applyStatisticsSettings(topo)
            topo.addParams(self._getGroupParams("main"))
            topo.addParams({ "max_path_length" : self.max_path_length})