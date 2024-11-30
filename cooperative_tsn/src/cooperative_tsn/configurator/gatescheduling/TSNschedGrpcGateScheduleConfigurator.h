//
// Copyright (C) 2024 Jannusch Bigge <jannusch.bigge@tu-dresden.de>
//
// Documentation for these modules is at http://veins.car2x.org/
//
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include "inet/linklayer/configurator/gatescheduling/base/GateScheduleConfiguratorBase.h"
#include "inet/linklayer/configurator/gatescheduling/common/TSNschedGateScheduleConfigurator.h"

namespace inet {

/**
 * @brief
 * Calls TSNsched via gRPC and returns the schedule, if scheduling is possible.
 */
class INET_API TSNschedGrpcGateScheduleConfigurator : public TSNschedGateScheduleConfigurator {
protected:
    /**
     * @brief
     * Converts an INET input into a json object.
     * In the original implementation from INET some strings were not surrounded by " which leads to problems with the java and python json loader.
     */
    virtual cValueMap* convertInputToJson(const Input& input) const override;

    virtual void writeInputToFile(const Input& input, std::string fileName) const override;

    /**
     * @brief
     * Executes the scheduler with a given input.
     * Calls a python script that acts as a gRPC client.
     * In future this should be replaced by a C++ gRPC client implementation.
     * In case of an error - connection to the scheduler, or unsuccessful scheduling - an exception is thrown.
     * In future the scheduler returns an error if the scheduling wasn't successful.
     */
    virtual void executeTSNsched(std::string fileName) const override;

    /**
     * @brief
     * Stores the calculated configuration in a json file.
     * The name of the file is the same as the name from the input file with an appended ".output".
     * In case that the scheduling wasn't successful, an exception is thrown in @executeTSNsched that will not be handled by now.
     */
    virtual Output* computeGateScheduling(const Input& input) const override;
};

} // namespace inet
