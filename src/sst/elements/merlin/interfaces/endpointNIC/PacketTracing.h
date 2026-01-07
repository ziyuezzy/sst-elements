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

#ifndef COMPONENTS_MERLIN_PACKETTRACING_H
#define COMPONENTS_MERLIN_PACKETTRACING_H

#include "NICPlugin.h"
#include "../ExtendedRequest.h"
#include <sst/core/output.h>
#include <fstream>
#include <string>
#include <mutex>
#include <atomic>

namespace SST {
namespace Merlin {

// PacketTracingMetadata is defined in ExtendedRequest.h

class PacketTracingPlugin : public NICPlugin
{
private:
    // Shared packet ID counter across all endpoints (static atomic for runtime access)
    static std::atomic<uint64_t> global_packet_id;

    // Per-rank CSV file management (file I/O is not shared across MPI ranks)
    static std::ofstream csv_file;
    static std::mutex csv_mutex;
    static bool csv_initialized;

    SST::Interfaces::SimpleNetwork::nid_t endpoint_id;
    Output output;
    bool enable_tracing;

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        PacketTracingPlugin,
        "merlin",
        "packetTracingPlugin",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Packet tracing plugin that logs packet injection/ejection events to CSV",
        SST::Merlin::NICPlugin
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"csv_filename", "Output CSV filename for packet traces", "packet_trace.csv"},
        {"enable_tracing", "Enable/disable packet tracing", "true"}
    )

    PacketTracingPlugin(ComponentId_t cid, Params& params);
    virtual ~PacketTracingPlugin();

    // NICPlugin interface
    virtual SST::Interfaces::SimpleNetwork::Request* processOutgoing(
        SST::Interfaces::SimpleNetwork::Request* req, int vn) override;

    virtual SST::Interfaces::SimpleNetwork::Request* processIncoming(
        SST::Interfaces::SimpleNetwork::Request* req, int vn) override;

    virtual std::string getPluginName() const override { return "PacketTracingPlugin"; }

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
