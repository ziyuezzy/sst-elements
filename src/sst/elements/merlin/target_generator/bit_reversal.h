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


#ifndef COMPONENTS_MERLIN_TARGET_GENERATOR_BIT_REVERSAL_H
#define COMPONENTS_MERLIN_TARGET_GENERATOR_BIT_REVERSAL_H

#include <sst/elements/merlin/target_generator/target_generator.h>
#include <math.h>

namespace SST {
namespace Merlin {


class BitReversalDist : public TargetGenerator {

private:

    unsigned int reverseBits(int num) {
        assert(num>=0 && log2(num)<=num_bits);
        unsigned int reversedNum = 0;
        for (unsigned int i = 0; i < num_bits; i++) {
            if ((num & (1 << i)) != 0) {
                reversedNum |= 1 << ((num_bits - 1) - i);
            }
        }
        return reversedNum;
    }

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        BitReversalDist,
        "merlin",
        "targetgen.bit_reversal",
        SST_ELI_ELEMENT_VERSION(0,0,1),
        "Generates a generalized bit reversal pattern. (d_i=s_(b-i-1)).",
        SST::Merlin::TargetGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
    )

    int dest;
    int num_bits;

public:

    BitReversalDist(ComponentId_t cid, Params &params, int id, int num_peers) :
        TargetGenerator(cid)
    {
        dest=-1;
        num_bits=ceil(log2(num_peers));
        if(reverseBits(id)<num_peers)
            dest=reverseBits(id);
    }

    ~BitReversalDist() {
    }

    void initialize(int id, int num_peers) {
        dest=-1;
        num_bits=ceil(log2(num_peers));
        if(reverseBits(id)<num_peers)
            dest=reverseBits(id);
        // dest = num_peers - 1 - id;  //why is this needed??
    }

    int getNextValue(void) {
        return dest;
    }

    void seed(uint32_t val) {
    }
};

} //namespace Merlin
} //namespace SST

#endif
