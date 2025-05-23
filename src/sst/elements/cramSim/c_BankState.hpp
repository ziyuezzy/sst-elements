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

// Copyright 2016 IBM Corporation

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//   http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef C_BANKSTATE_HPP
#define C_BANKSTATE_HPP

#include <sst/core/sst_types.h>

// C++ includes
#include <memory>
#include <list>
#include <map>

// CramSim includes
//#include "c_BankCommand.hpp"
//#include "c_BankInfo.hpp"

namespace SST {
namespace CramSim {

class c_BankInfo;
class c_BankCommand;
enum class e_BankCommandType;

// Bankstates
enum class e_BankState {
    NONE, IDLE, ACTNG, ACTIVE, READ, READA, WRITE, WRITEA, PRE, REF
};
// NONE state is for the startup situation when IDLE state is entered.



class c_BankState {
public:
    virtual ~c_BankState() {
    }

    virtual void handleCommand(c_BankInfo* x_bank, c_BankCommand* x_bankCommandPtr, SimTime_t x_cycle) = 0;

    virtual void clockTic(c_BankInfo* x_bank, SimTime_t x_cycle) = 0;

    virtual void enter(c_BankInfo* x_bank, c_BankState* x_prevState,
            c_BankCommand* x_cmdPtr, SimTime_t x_cycle) = 0;

    virtual std::list<e_BankCommandType> getAllowedCommands() = 0;

    virtual bool isCommandAllowed(c_BankCommand* x_cmdPtr,
            c_BankInfo* x_bankPtr) = 0;

    e_BankState getCurrentState() {
        return m_currentState;
    }

//private:
protected:
    std::map<std::string, unsigned>* m_bankParams;

    e_BankState m_currentState;

};
}
}

#endif // C_BANKSTATE_HPP
