// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_MERLIN_DEMANDMATRIX_H
#define COMPONENTS_MERLIN_DEMANDMATRIX_H

#include "NICPlugin.h"
#include <sst/core/output.h>
#include <sst/core/statapi/stataccumulator.h>
#include <map>
#include <string>

namespace SST {
namespace Merlin {

class DemandMatrixPlugin : public NICPlugin
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        DemandMatrixPlugin,
        "merlin",
        "demandMatrixPlugin",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Demand Matrix plugin that periodically records endpoint-to-endpoint traffic demands",
        SST::Merlin::NICPlugin
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"metric", "What to measure: 'packets', 'bytes', or 'bits'", "bytes"},
        {"endpoint_router_mapping", "Mapping of endpoint IDs to router IDs (used to determine number of endpoints)", ""},
        {"EP_id", "Endpoint ID (set by endpointNIC)", "-1"}
    )

    SST_ELI_DOCUMENT_STATISTICS(
        {"demand_matrix", "Traffic demand from this endpoint to each destination", "bytes", 1}
    )

    DemandMatrixPlugin(ComponentId_t cid, Params& params);
    virtual ~DemandMatrixPlugin();

    // NICPlugin interface
    virtual SST::Interfaces::SimpleNetwork::Request* processOutgoing(
        SST::Interfaces::SimpleNetwork::Request* req, int vn) override;

    virtual SST::Interfaces::SimpleNetwork::Request* processIncoming(
        SST::Interfaces::SimpleNetwork::Request* req, int vn) override;

    virtual std::string getPluginName() const override { return "DemandMatrixPlugin"; }

    virtual void plugin_init(unsigned int phase) override;
    virtual void plugin_setup() override;
    virtual void plugin_finish() override;

private:
    // Configuration
    SST::Interfaces::SimpleNetwork::nid_t endpoint_id;
    int num_endpoints;

    enum MetricType {
        PACKETS,
        BYTES,
        BITS
    } metric_type;

    Output output;

    // Statistics - one per destination endpoint
    // Key: destination endpoint ID, Value: statistic pointer
    std::map<SST::Interfaces::SimpleNetwork::nid_t, Statistic<uint64_t>*> dest_statistics;

    // Helper methods
    uint64_t getTrafficAmount(SST::Interfaces::SimpleNetwork::Request* req);
    Statistic<uint64_t>* getStatistic(SST::Interfaces::SimpleNetwork::nid_t dst);
};

} // namespace Merlin
} // namespace SST

#endif
