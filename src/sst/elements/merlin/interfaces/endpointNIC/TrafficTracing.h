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

#ifndef COMPONENTS_MERLIN_TRAFFICTRACING_H
#define COMPONENTS_MERLIN_TRAFFICTRACING_H

#include "NICPlugin.h"
#include "../ExtendedRequest.h"
#include <sst/core/output.h>
#include <fstream>
#include <string>
#include <atomic>
#include <mutex>

namespace SST {
namespace Merlin {

// TrafficTracingMetadata is now defined in ExtendedRequest.h

class TrafficTracingPlugin : public NICPlugin
{
private:
    // Static shared packet ID counter across all endpoints (atomic for thread safety)
    static std::atomic<uint64_t> global_packet_id;

    // Static shared CSV file handle and mutex for thread-safe I/O
    static std::ofstream csv_file;
    static std::mutex csv_mutex;
    static bool csv_initialized;
    static std::string csv_filename;

    SST::Interfaces::SimpleNetwork::nid_t endpoint_id;
    Output output;
    bool enable_tracing;

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        TrafficTracingPlugin,
        "merlin",
        "trafficTracingPlugin",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Traffic tracing plugin that logs packet injection/ejection events to CSV",
        SST::Merlin::NICPlugin
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"csv_filename", "Output CSV filename for traffic traces", "traffic_trace.csv"},
        {"enable_tracing", "Enable/disable traffic tracing", "true"}
    )

    TrafficTracingPlugin(ComponentId_t cid, Params& params);
    virtual ~TrafficTracingPlugin();

    // NICPlugin interface
    virtual SST::Interfaces::SimpleNetwork::Request* processOutgoing(
        SST::Interfaces::SimpleNetwork::Request* req, int vn) override;

    virtual SST::Interfaces::SimpleNetwork::Request* processIncoming(
        SST::Interfaces::SimpleNetwork::Request* req, int vn) override;

    virtual std::string getPluginName() const override { return "TrafficTracingPlugin"; }

    virtual void plugin_init(unsigned int phase) override;
    virtual void plugin_finish() override;

private:
    void initCSV(const std::string& filename);
    void logPacketEvent(const std::string& event_type, uint64_t pkt_id,
                       SST::Interfaces::SimpleNetwork::nid_t src,
                       SST::Interfaces::SimpleNetwork::nid_t dest,
                       size_t size_bytes);
    uint64_t assignPacketID();
};

} // namespace Merlin
} // namespace SST

#endif
