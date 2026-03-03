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

#include <sst_config.h>
#include "DemandMatrix.h"
#include <sstream>
#include <iomanip>

namespace SST {
namespace Merlin {

DemandMatrixPlugin::DemandMatrixPlugin(ComponentId_t cid, Params& params) :
    NICPlugin(cid, params),
    endpoint_id(-1)
{
    output.init(getName() + ": ", 0, 0, Output::STDOUT);

    // Get endpoint ID from params (set by endpointNIC)
    endpoint_id = params.find<SST::Interfaces::SimpleNetwork::nid_t>("EP_id", -1);
    if (endpoint_id == static_cast<SST::Interfaces::SimpleNetwork::nid_t>(-1)) {
        output.fatal(CALL_INFO, -1, "DemandMatrixPlugin requires 'EP_id' parameter to be set\n");
    }

    // Get total number of endpoints
    num_endpoints = params.find<int>("num_peers", -1);
    if (num_endpoints == -1) {
        output.fatal(CALL_INFO, -1, "DemandMatrixPlugin requires 'num_peers' parameter\n");
    }

    // Get metric type
    std::string metric_str = params.find<std::string>("metric", "bytes");
    if (metric_str == "packets") {
        metric_type = PACKETS;
    } else if (metric_str == "bytes") {
        metric_type = BYTES;
    } else if (metric_str == "bits") {
        metric_type = BITS;
    } else {
        output.fatal(CALL_INFO, -1,
            "DemandMatrixPlugin: invalid metric '%s'. Must be 'packets', 'bytes', or 'bits'\n",
            metric_str.c_str());
    }

    // Pre-register statistics for all possible destinations
    for (int dst = 0; dst < num_endpoints; dst++) {
        std::string sub_id = "dst_" + std::to_string(dst);
        Statistic<uint64_t>* stat = registerStatistic<uint64_t>("demand_matrix", sub_id);
        dest_statistics[dst] = stat;
    }

    output.verbose(CALL_INFO, 1, 0,
        "DemandMatrixPlugin: EP %ld initialized with %d total endpoints, pre-registered %d statistics\n",
        endpoint_id, num_endpoints, num_endpoints);

    output.verbose(CALL_INFO, 1, 0,
        "DemandMatrixPlugin initialized for endpoint %ld with %d total endpoints, metric=%s\n",
        endpoint_id, num_endpoints, metric_str.c_str());
}

DemandMatrixPlugin::~DemandMatrixPlugin()
{
    // Clock handler is automatically cleaned up by SST
}

void DemandMatrixPlugin::plugin_init(unsigned int phase)
{
    // Nothing needed for init
}

void DemandMatrixPlugin::plugin_setup()
{
    // Note: We can't pre-register all destination statistics here because
    // we don't know the total number of endpoints. Statistics will be
    // registered on-demand during runtime, which works as long as
    // enableAllStatistics is called before setup completes.
    output.verbose(CALL_INFO, 1, 0, "DemandMatrixPlugin setup complete\n");
}

void DemandMatrixPlugin::plugin_finish()
{
    // Statistics framework handles data output automatically
    output.verbose(CALL_INFO, 1, 0,
        "DemandMatrixPlugin finished for endpoint %ld\n", endpoint_id);
}

SST::Interfaces::SimpleNetwork::Request* DemandMatrixPlugin::processOutgoing(
    SST::Interfaces::SimpleNetwork::Request* req, int vn)
{
    if (!req) return nullptr;

    // Get statistic for this destination
    SST::Interfaces::SimpleNetwork::nid_t dst = req->dest;
    Statistic<uint64_t>* stat = getStatistic(dst);

    if (stat) {
        // Record traffic to this destination
        uint64_t amount = getTrafficAmount(req);
        stat->addData(amount);

        output.verbose(CALL_INFO, 2, 0, "Recording %lu %s to dst %ld\n",
                       amount, (metric_type == BYTES ? "bytes" : (metric_type == BITS ? "bits" : "packets")), dst);
    }

    // Pass through unchanged
    return req;
}

SST::Interfaces::SimpleNetwork::Request* DemandMatrixPlugin::processIncoming(
    SST::Interfaces::SimpleNetwork::Request* req, int vn)
{
    // We only track outgoing traffic to avoid double-counting
    // Pass through unchanged
    return req;
}

Statistic<uint64_t>* DemandMatrixPlugin::getStatistic(SST::Interfaces::SimpleNetwork::nid_t dst)
{
    // All statistics are pre-registered in constructor
    auto it = dest_statistics.find(dst);
    if (it != dest_statistics.end()) {
        return it->second;
    }

    // If destination wasn't pre-registered, log warning and return nullptr
    output.verbose(CALL_INFO, 1, 0, "Warning: destination %ld was not pre-registered\n", dst);
    return nullptr;
}

uint64_t DemandMatrixPlugin::getTrafficAmount(SST::Interfaces::SimpleNetwork::Request* req)
{
    switch (metric_type) {
        case PACKETS:
            return 1; // Count as 1 packet

        case BYTES:
            // Size is in bits, convert to bytes
            return (req->size_in_bits + 7) / 8;

        case BITS:
            return req->size_in_bits;

        default:
            return 0;
    }
}

} // namespace Merlin
} // namespace SST
