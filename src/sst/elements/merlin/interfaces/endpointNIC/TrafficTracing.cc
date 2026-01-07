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
#include "TrafficTracing.h"
#include <sstream>
#include <iomanip>

namespace SST {
namespace Merlin {

// Initialize static members
std::atomic<uint64_t> TrafficTracingPlugin::global_packet_id(0);
std::ofstream TrafficTracingPlugin::csv_file;
std::mutex TrafficTracingPlugin::csv_mutex;
bool TrafficTracingPlugin::csv_initialized = false;
std::string TrafficTracingPlugin::csv_filename = "";

TrafficTracingPlugin::TrafficTracingPlugin(ComponentId_t cid, Params& params) :
    NICPlugin(cid, params),
    endpoint_id(-1)
{
    output.init(getName() + ": ", 0, 0, Output::STDOUT);

    // Get endpoint ID from params
    endpoint_id = params.find<SST::Interfaces::SimpleNetwork::nid_t>("EP_id", -1);
    if (endpoint_id == static_cast<SST::Interfaces::SimpleNetwork::nid_t>(-1)) {
        output.fatal(CALL_INFO, -1, "EP_id parameter not provided to TrafficTracingPlugin\n");
    }

    // Get CSV filename (only initialize once, with mutex protection)
    std::string filename = params.find<std::string>("csv_filename", "traffic_trace.csv");

    // Use mutex to ensure only one thread initializes the CSV file
    std::lock_guard<std::mutex> lock(csv_mutex);
    if (!csv_initialized) {
        csv_filename = filename;
        csv_initialized = true;
        initCSV(filename);
    }
}

TrafficTracingPlugin::~TrafficTracingPlugin() {
}

void TrafficTracingPlugin::plugin_init(unsigned int phase) {
    // Nothing to do during init phases
}

void TrafficTracingPlugin::plugin_finish() {
    // Close CSV file on finish (only once, with mutex protection)
    std::lock_guard<std::mutex> lock(csv_mutex);
    if (csv_file.is_open() && endpoint_id == 0) {
        csv_file.close();
        output.verbose(CALL_INFO, 1, 0, "Closed traffic trace CSV file\n");
    }
}

void TrafficTracingPlugin::initCSV(const std::string& filename) {
    // Note: This method should only be called when csv_mutex is already locked
    csv_file.open(filename, std::ios::out | std::ios::trunc);

    if (!csv_file.is_open()) {
        output.fatal(CALL_INFO, -1, "Failed to open CSV file for traffic tracing: %s\n",
                    filename.c_str());
    }

    // Write CSV header
    csv_file << "time_ns,srcNIC,destNIC,Size_Bytes,pkt_id,event_type\n";
    csv_file.flush();

    output.verbose(CALL_INFO, 1, 0, "Initialized traffic trace CSV: %s\n", filename.c_str());
}

uint64_t TrafficTracingPlugin::assignPacketID() {
    return global_packet_id.fetch_add(1);
}

void TrafficTracingPlugin::logPacketEvent(
    const std::string& event_type,
    uint64_t pkt_id,
    SST::Interfaces::SimpleNetwork::nid_t src,
    SST::Interfaces::SimpleNetwork::nid_t dest,
    size_t size_bytes)
{
    // Use mutex to ensure thread-safe file I/O
    std::lock_guard<std::mutex> lock(csv_mutex);

    if (!csv_file.is_open()) {
        return;
    }

    // Get current simulation time in nanoseconds
    SimTime_t current_time = getCurrentSimTimeNano();

    // Write CSV entry atomically
    csv_file << current_time << ","
             << src << ","
             << dest << ","
             << size_bytes << ","
             << pkt_id << ","
             << event_type << "\n";

    // Flush to ensure data is written immediately (important for crash recovery)
    csv_file.flush();
}

SST::Interfaces::SimpleNetwork::Request* TrafficTracingPlugin::processOutgoing(
    SST::Interfaces::SimpleNetwork::Request* req, int vn)
{
    if (!req) return nullptr;

    // Convert to ExtendedRequest if not already
    ExtendedRequest* ext_req = dynamic_cast<ExtendedRequest*>(req);
    if (!ext_req) {
        ext_req = new ExtendedRequest(req);
        delete req;
    }

    // Assign packet ID and store in metadata
    uint64_t pkt_id = assignPacketID();
    TrafficTracingMetadata trace_meta(pkt_id);
    ext_req->setMetadata("TrafficTracing", trace_meta);

    // Log injection event (packet going IN to the network)
    size_t size_bytes = (ext_req->size_in_bits + 7) / 8;  // Convert bits to bytes
    logPacketEvent("in", pkt_id, ext_req->src, ext_req->dest, size_bytes);

    return ext_req;
}

SST::Interfaces::SimpleNetwork::Request* TrafficTracingPlugin::processIncoming(
    SST::Interfaces::SimpleNetwork::Request* req, int vn)
{
    if (!req) return nullptr;

    // Try to get ExtendedRequest to retrieve packet ID
    ExtendedRequest* ext_req = dynamic_cast<ExtendedRequest*>(req);

    if (ext_req) {
        // Retrieve packet ID from metadata
        TrafficTracingMetadata trace_meta;
        if (ext_req->getMetadata("TrafficTracing", trace_meta)) {
            // Log ejection event (packet coming OUT of the network)
            size_t size_bytes = (ext_req->size_in_bits + 7) / 8;  // Convert bits to bytes
            logPacketEvent("out", trace_meta.pkt_id, ext_req->src, ext_req->dest, size_bytes);
        } else {
            output.verbose(CALL_INFO, 2, 0,
                "Warning: Incoming packet missing TrafficTracing metadata\n");
        }
    }

    return req;
}

} // namespace Merlin
} // namespace SST
