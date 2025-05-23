// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

#include <sst_config.h>
#include <string>

#include "messier.h"
#include "messier_event.h"

using namespace SST;
using namespace SST::MessierComponent;


#define MESSIER_VERBOSE(LEVEL, OUTPUT) if(verbosity >= (LEVEL)) OUTPUT

void Messier::parser(NVM_PARAMS * nvm, SST::Params& params)
{
    nvm->size = (uint32_t) params.find<uint32_t>("size", 8388608);  // in KB, which mean 8GB
    nvm->write_buffer_size = (uint32_t) params.find<uint32_t>("write_buffer_size", 128); ;
    nvm->max_outstanding = (uint32_t) params.find<uint32_t>("max_outstanding", 16) ;
    nvm->max_current_weight = (uint32_t) params.find<uint32_t>("max_current_weight", 120); ;
    nvm->write_weight = (uint32_t) params.find<uint32_t>("write_weight", 15); ;
    nvm->read_weight = (uint32_t) params.find<uint32_t>("read_weight", 3) ;
    nvm->max_writes = (uint32_t) params.find<uint32_t>("max_writes", 1) ;

    uint32_t cache_interleave = (uint32_t) params.find<uint32_t>("cacheline_interleaving", 1) ;
    uint32_t adaptive_writes = (uint32_t) params.find<uint32_t>("adaptive_writes", 0) ;
    uint32_t cache_enabled = (uint32_t) params.find<uint32_t>("cache_enabled", 0) ;
    uint32_t write_cancel = (uint32_t) params.find<uint32_t>("write_cancel", 0) ;
    uint32_t write_cancel_th = (uint32_t) params.find<uint32_t>("write_cancel_th", 0) ;
    uint32_t modulo = (uint32_t) params.find<uint32_t>("modulo", 0) ;
    uint32_t modulo_unit = (uint32_t) params.find<uint32_t>("modulo_unit", 4) ;

    nvm->modulo_unit = modulo_unit;

    if(modulo)
        nvm->modulo = true;
    else
        nvm->modulo = false;

    if(write_cancel)
        nvm->write_cancel = true;
    else
        nvm->write_cancel = false;

    nvm->write_cancel_th = write_cancel_th;

    if(cache_enabled)
        nvm->cache_enabled = true;
    else
        nvm->cache_enabled = false;

    int cache_persistent = (uint32_t) params.find<uint32_t>("cache_persistent", 0) ;

    if(cache_persistent)
        nvm->cache_persistent = true;
    else
        nvm->cache_persistent = false;


    nvm->cache_latency = (uint32_t) params.find<uint32_t>("cache_latency", 0) ;

    nvm->cache_size = (uint32_t) params.find<uint32_t>("cache_size", 0) ;

    nvm->cache_assoc = (uint32_t) params.find<uint32_t>("cache_assoc", 0) ;

    nvm->cache_bs = (uint32_t) params.find<uint32_t>("cache_bs", 0) ;

    if(cache_interleave==1)
        nvm->cacheline_interleaving = true;

    if(adaptive_writes==1)
        nvm->adaptive_writes = true;
    else
        nvm->adaptive_writes = false;


    nvm->tCMD = (uint32_t) params.find<uint32_t>("tCMD", 1); ;

    nvm->tCL = (uint32_t) params.find<uint32_t>("tCL", 70); ;

    nvm->tRCD = (uint32_t) params.find<uint32_t>("tRCD", 300) ;

    std::cout<<"The value of tRCD is "<<nvm->tRCD<<std::endl;

    nvm->tCL_W = (uint32_t) params.find<uint32_t>("tCL_W", 1000) ;

    nvm->tBURST = (uint32_t) params.find<uint32_t>("tBURST", 7); ;

    nvm->device_width = (uint32_t) params.find<uint32_t>("device_width", 8) ;

    nvm->num_ranks = (uint32_t) params.find<uint32_t>("num_ranks", 1); ;

    nvm->num_devices = (uint32_t) params.find<uint32_t>("num_devices", 8) ;
    nvm->num_banks = (uint32_t) params.find<uint32_t>("num_banks", 16);
    nvm->row_buffer_size = (uint32_t) params.find<uint32_t>("row_buffer_size", 8192) ;
    nvm->flush_th = (uint32_t) params.find<uint32_t>("flush_th", 50) ;
    nvm->flush_th_low = (uint32_t) params.find<uint32_t>("flush_th_low", 30) ;
    nvm->max_requests = (uint32_t) params.find<uint32_t>("max_requests", 32);

    nvm->group_size = (uint32_t) params.find<uint32_t>("group_size", nvm->num_banks) ;

    nvm->lock_period = (uint32_t) params.find<uint32_t>("lock_period", 10000) ;

}

// Here we do the initialization of the Samba units of the system, connecting them to the cores and instantiating TLB hierachy objects for each one

Messier::Messier(SST::ComponentId_t id, SST::Params& params): Component(id) {
    char* link_buffer = (char*) malloc(sizeof(char) * 256);
        size_t buffer_size = sizeof(char) * 256;

    snprintf(link_buffer, buffer_size, "bus");


    nvm_params = new NVM_PARAMS();

    parser(nvm_params, params);


    DIMM = loadComponentExtension<NVM_DIMM>(*nvm_params);

        m_memChan = configureLink(link_buffer, "1ns", new Event::Handler2<NVM_DIMM,&NVM_DIMM::handleRequest>(DIMM));


    snprintf(link_buffer, buffer_size, "event_bus");

        event_link = configureSelfLink(link_buffer, "1ns", new Event::Handler2<NVM_DIMM,&NVM_DIMM::handleEvent>(DIMM));


    DIMM->setMemChannel(m_memChan);

    DIMM->setEventChannel(event_link);

    std::cout<<"After initialization "<<std::endl;

    std::string cpu_clock = params.find<std::string>("clock", "1GHz");

    TimeConverter tc = getTimeConverter(cpu_clock);
        event_link->setDefaultTimeBase(tc);


    registerClock( cpu_clock, new Clock::Handler2<Messier,&Messier::tick>(this) );

}

Messier::Messier() : Component(-1)
{
    // for serialization only
    //
}

bool Messier::tick(SST::Cycle_t x)
{
    // We tick the MMU hierarchy of each core
    DIMM->tick();

    return false;
}
