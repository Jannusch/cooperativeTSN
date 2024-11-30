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

#pragma once

#include <cmath>
#include <memory>
#include <string>
#include <sys/types.h>
#include <vector>

#include "cooperative_tsn/applications/NicForwardingApplication.h"

#include "inet/common/Units.h"

#include "omnetpp/csimulation.h"
#include "omnetpp/simtime.h"

namespace veins {

class DynSchedForwardingApplication : public NicForwardingApplication {

    class TsnStream : cModule {
    private:
        // Values from the "official" TSN configuration
        std::string streamName;
        int PCP;
        int gateIndex;
        cModule* applicationModule;
        cModule* sourceModule;
        cModule* externalSourceModule;
        cModule* destinationModule;
        inet::b packetSize;
        inet::ms packetInterval;
        inet::ms maxLatency;
        inet::ms maxJitter;
        std::vector<cModule*> pathFragments;

        // Internal parameters
        std::deque<simtime_t> lastTimePacketArrived;
        int queueSize = 10;
        simtime_t startTime = SimTime::ZERO;
        double dataRate = 100; // 100 Mbits
        const int updateIntervall = 10;
        int numberPackets = 0;
        inet::b originalPacketSize;

    public:
        // constructor to be called from the application during runtime
        // TsnStream(std::string streamName, uint8_t PCP, uint8_t gateIndex, std::string applicationModule, std::string sourceModule, std::string destinationModule, inet::b packetSize, inet::ms packetInterval, inet::ms maxLatency, inet::ms maxJitter = inet::ms(0), std::vector<cModule*> pathFragments = {})
        // : streamName(streamName), PCP(PCP), gateIndex(gateIndex), packetSize(packetSize), packetInterval(packetInterval), maxLatency(maxLatency), maxJitter(maxJitter), pathFragments(pathFragments) {};
        // constructor for configuration from omnetpp.ini
        TsnStream()
        {
        }
        ~TsnStream()
        {
        }
        TsnStream(TsnStream* old)
            : streamName(old->streamName)
            , PCP(old->PCP)
            , gateIndex(old->gateIndex)
            , applicationModule(old->applicationModule)
            , sourceModule(old->sourceModule)
            , externalSourceModule(old->externalSourceModule)
            , destinationModule(old->destinationModule)
            , packetSize(old->packetSize)
            , packetInterval(old->packetInterval)
            , maxLatency(old->maxLatency)
            , maxJitter(old->maxJitter)
            , pathFragments(old->pathFragments)
            , lastTimePacketArrived(old->lastTimePacketArrived)
            , queueSize(old->queueSize)
            , dataRate(old->dataRate)
            , originalPacketSize(old->originalPacketSize)
        {
        }
        TsnStream(cValueMap* configLine, cModule* parent, cModule* externalSourceModule = nullptr)
        {

            // mandatory fields
            auto parent_full_path = parent->getFullPath();
            // source
            sourceModule = parent->getModuleByPath((parent_full_path + "." + configLine->get("source").stringValue()).c_str());
            ASSERT2(sourceModule, "source module not found");

            // external source
            this->externalSourceModule = externalSourceModule;

            // destination
            destinationModule = parent->getModuleByPath((parent_full_path + "." + configLine->get("destination").stringValue()).c_str());
            ASSERT2(destinationModule, "destination module not found");

            // pcp
            PCP = configLine->get("pcp").intValue();
            ASSERT2(PCP >= 0 && PCP <= 7, "PCP must be between 0 and 7");

            // gateIndex
            gateIndex = configLine->get("gateIndex").intValue();
            ASSERT2(gateIndex >= 0, "gateIndex must be greater than 0");

            // packetSize
            packetSize = inet::b(configLine->get("packetLength").doubleValueInUnit("b"));
            ASSERT2(packetSize.get() > 0, "packetSize must be greater than 0");
            originalPacketSize = packetSize;

            // packetInterval
            packetInterval = inet::ms(configLine->get("packetInterval").doubleValueInUnit("ms"));
            // ASSERT2(packetInterval.get() > 0, "packetInterval must be greater than 0");

            // optional ones
            for (auto key : configLine->getFields()) {
                if (key.first == "streamName") {
                    streamName = key.second.str();
                }
                else if (key.first == "application") {
                    // check if this works with vectors
                    auto app_name = std::string(key.second.stringValue());
                    if (app_name.find("[") != std::string::npos) {
                        auto app_name_split = app_name.substr(0, app_name.find("["));
                        auto app_index = std::stoi(app_name.substr(app_name.find("[") + 1, app_name.find("]") - app_name.find("[") - 1));
                        applicationModule = sourceModule->getSubmodule(app_name_split.c_str(), app_index);
                    }
                    else {
                        applicationModule = parent->getSubmodule(key.second.stringValue());
                    }
                }
                else if (key.first == "maxLatency") {
                    maxLatency = inet::ms(key.second.doubleValueInUnit("ms"));
                }
                else if (key.first == "maxJitter") {
                    maxJitter = inet::ms(key.second.doubleValueInUnit("ms"));
                }
                else if (key.first == "pathFragments") {
                    auto valueArray = check_and_cast<cValueArray*>(key.second.objectValue());
                    auto parent_path = parent->getFullPath();
                    for (auto pathFragment : valueArray->getArray()) {
                        auto path = parent_path + "." + pathFragment.stringValue();
                        pathFragments.push_back(parent->getModuleByPath(path.c_str()));
                    }
                }
            }

            for (int i = 0; i < queueSize; i++) {
                lastTimePacketArrived.push_back(SimTime::ZERO);
            }

            // set stream name if not set
            if (streamName.empty()) {
                // TODO: I should add the application as well to be able to distinguish multiple external partners from the same external interface. But I think for the configuration this is not relevant. But should inspect this in the future
                std::stringstream ss;
                if (externalSourceModule) {
                    ss << externalSourceModule->getParentModule()->getFullName() << "." << externalSourceModule->getFullName() << "-";
                }
                ;
                ss << sourceModule->getName() << "-PCP" << PCP << "-" << destinationModule->getName();
                streamName = ss.str();
            }
        }

        friend std::ostream& operator<<(std::ostream& os, const TsnStream& stream)
        {
            os << "{"
                << "name: \"" << stream.streamName << "\", "
                << "pcp: " << stream.PCP << ", "
                << "gateIndex: " << stream.gateIndex << ", ";
            if (stream.applicationModule->isVector()) {
                os << "application: \"" << stream.applicationModule->getName() << "[" << stream.applicationModule->getIndex() << "]\", ";
            }
            else {
                os << "application: \"" << stream.applicationModule->getName() << "\", ";
            }
            os << "source: \"" << stream.sourceModule->getName() << "\", "
                << "destination: \"" << stream.destinationModule->getName() << "\", "
                << "packetLength: " << stream.packetSize << ", "
                << "packetInterval: " << stream.packetInterval;
            // optional fields
            if (stream.maxLatency.get() > 0) {
                os << ", "
                    << "maxLatency: " << stream.maxLatency;
            }
            if (stream.maxJitter.get() > 0) {
                os << ", "
                    << "maxJitter: " << stream.maxJitter;
            }
            // start time
            os << ", "
                << "startTime: " << stream.startTime;
            if (stream.pathFragments.size() > 0) {
                os << ", "
                    << "pathFragments: [";
                for (auto pathFragment : stream.pathFragments) {
                    os << "\"" << pathFragment->getName() << "\", ";
                }
                os << "]";
            }
            os << "}";
            return os;
        }

    public:
        inet::b getNewSlotSize()
        {

            // New Version:
            // we use 10% as a saftet range
            // I not only have to increase the range, but set the start time as well

            // this function needs to calculate the slotsize
            // for now we can not shift it into the past, and therfore we dont need some cool calculations because the scheduler can't handle it
            // quite a few things are not so good. eg the speed of the connections is set by a parameter but finaley the slot size depends of the speed of each individual connection (and is set so by the configurator) but there is simply no way to set the slot size by time

            // check the update intervall and we want to have the posibillity to init streams with an intervall of zero
            if (numberPackets % updateIntervall != 2) {
                return packetSize;
            }

            SimTime min = SimTime::ZERO;
            SimTime max = SimTime::ZERO;
            SimTime sum;

            for (auto time : lastTimePacketArrived) {
                if (time == SimTime::ZERO) {
                    continue;
                }

                if (time < min || min == SimTime::ZERO) {
                    min = time;
                }
                if (time > max || max == SimTime::ZERO) {
                    max = time;
                }
                sum += time;
            }
            startTime = min;
            auto diff_time = max - min;

            auto tmp_max = max.str();
            auto tmp_min = min.str();
            auto tmp8 = diff_time.str();

            auto packet_size = originalPacketSize.get(); // this is bits but data rate should be bits as well
            // we convert it to ns so that we have the precision for bits and with 100 Mbps (I'm not sure if this is a good solution)
            // TODO: this looks broken the time isn't long enough as that the gate will pull the last packet out of the queue (again something that prob. would be discovered if we would have correct slot sizes...)
            auto packetTime = SimTime((originalPacketSize.get() / dataRate) * 1000, omnetpp::SIMTIME_NS);
            auto tmp5 = packetTime.str();
            // TODO: this needs to be adjusted because this is ugly hard coded (latency of the store and forwarde)
            auto total_time = diff_time;

            // Adjust slot size by 10%
            auto adjustment = total_time * 0.1;
            auto start_offset = adjustment / 2;
            // set new start time
            this->adjusteStartTime(start_offset);
            total_time += adjustment;
            // this adjustment contains the offset needed for the loss in precision due to rounding as well as additional time needed to compensate the store and forwarde switching
            total_time += SimTime(21, omnetpp::SIMTIME_US) + packetTime;

            auto tmp7 = (diff_time + packetTime).str();
            auto tmp6 = total_time.str();
            // I need to check this value.
            auto slot_size = (total_time.inUnit(omnetpp::SIMTIME_NS) * dataRate / 1000);

            auto result = inet::b(std::ceil(slot_size));
            return result; // to get bits back
            // TODO: I need to create a test for this function and check if the slot size is correct
        }

        void setPacketSize(inet::b size)
        {
            this->packetSize = size;
        }

        // removes oldest time and pushes new time
        void pushTime(SimTime time)
        {
            numberPackets++;
            inet::us cycletime = inet::ms(100);
            auto tmp4 = time.str();

            // this implies that the dyn sched application is allways the source module
            // TODO: restructure this in a way that the parent module can be accessed -> ISSUE #5
            auto moduleCreationTimeOffset = check_and_cast<DynSchedForwardingApplication*>(sourceModule->getSubmodule("app", 0))->getCreationTimeOffset();
            auto tmp5 = moduleCreationTimeOffset.str();
            auto tmp9 = time.trunc(omnetpp::SIMTIME_NS).str();
            auto diff_time = fmod(time.trunc(omnetpp::SIMTIME_NS), SimTime(cycletime.get(), omnetpp::SIMTIME_US));
            if (lastTimePacketArrived.size() == queueSize) {
                lastTimePacketArrived.pop_back();
            }
            // maybe this could also be done a few line above in one step
            // this abormination should be checked with some test cases (problems are maybe the edges of the cycle)
            auto new_time = fmod((diff_time + ((100 - moduleCreationTimeOffset))), SimTime(cycletime.get(), omnetpp::SIMTIME_US));
            auto tmp8 = new_time.str();
            lastTimePacketArrived.push_front(new_time);
        }

        void setStartTime()
        {
            auto min = SimTime::ZERO;
            for (auto time : lastTimePacketArrived) {
                // TODO: issue #2
                if ((time < min || min == SimTime::ZERO) && time != SimTime::ZERO) {
                    min = time;
                }
            }
            startTime = min;
        }

        void adjusteStartTime(SimTime offset)
        {
            // TODO; Cycletime hardcoded
            startTime = fmod((startTime - offset), SimTime(100, omnetpp::SIMTIME_MS));
        }

        cModule* getSourceModule()
        {
            return sourceModule;
        }

        cModule* getDestinationModule()
        {
            return destinationModule;
        }

        int getPCP()
        {
            return PCP;
        }

        std::string getStreamName()
        {
            return streamName;
        }

        cModule* getExternalSourceModule()
        {
            return externalSourceModule;
        }

        inet::ms getPacketInterval()
        {
            return packetInterval;
        }

        void setPacketInterval()
        {
            auto sum = SimTime::ZERO;
            for (auto time : lastTimePacketArrived) {
                sum += time;
            }
            auto average = sum / lastTimePacketArrived.size();
            // oh no danger! We can not use a value that is not a multiple of the cycle time
            // stupid me!
            // TODO: parameter for cycle time
            this->packetInterval = inet::ms(100);
        }
    };

private:
    cModule* tsnScheduler;
    std::vector<std::shared_ptr<TsnStream>> tsnStreams;
    double dataRate; // data rate in Mbits for the TSN streams
    // Test
    void testConfiguration();
    simtime_t creationTime; // time when the module is created (needed to get cycle start time)

protected:
    virtual void initialize(int stage) override;
    void socketDataArrived(inet::UdpSocket* socket, inet::Packet* packet) override;
    void adaptSchedule(inet::Packet* packet);
    void handleMessageWhenUp(omnetpp::cMessage* msg) override;
    simtime_t getCreationTimeOffset();

    std::string getSchedulerConfiguration();
};

} // namespace veins
