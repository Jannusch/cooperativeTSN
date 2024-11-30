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

//
// Application based on the veins inet base application to forward messages accros cars
//

#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>

#include "cooperative_tsn/applications/DynSchedForwardingApplication.h"

#include "inet/common/INETDefs.h"
#include "inet/common/InitStages.h"
#include "inet/common/Simsignals.h"
#include "inet/common/Units.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/configurator/gatescheduling/base/GateScheduleConfiguratorBase.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/common/L3AddressTag_m.h"

#include "omnetpp/cproperty.h"
#include "omnetpp/csimulation.h"
#include "omnetpp/simtime.h"

namespace veins {
Define_Module(DynSchedForwardingApplication);

void DynSchedForwardingApplication::initialize(int stage)
{

    if (stage == inet::INITSTAGE_LAST) {
        ASSERT2(hasPar("schedulerModule"), "tsnScheduler parameter not set");
        tsnScheduler = dynamic_cast<inet::GateScheduleConfiguratorBase*>(getModuleByPath(par("schedulerModule").stringValue()));
        ASSERT(tsnScheduler);

        ASSERT2(hasPar("dataRate"), "dataRate parameter not set");
        dataRate = par("dataRate").doubleValueInUnit("Mbps");

        // read configuration and add it to the schedule
        auto configuration = check_and_cast<cValueArray*>(tsnScheduler->par("configuration").objectValue());

        for (int i = 0; i < configuration->size(); i++) {
            auto configurationline = check_and_cast<cValueMap*>(configuration->get(i).objectValue());
            auto stream = std::make_shared<TsnStream>(new TsnStream(configurationline, this->getParentModule()->getParentModule()));
            tsnStreams.push_back(stream);
        }

        // testConfiguration();

        creationTime = simTime();
        auto tmp7 = creationTime.str();
        // set update interval
        // TODO: update interval

        // for the simulation study we delay the update based on the index of the car in the platoon
        auto index = getParentModule()->getParentModule()->getIndex();
        scheduleAt(simTime() + SimTime(index + 5, omnetpp::SIMTIME_S) + SimTime(100, SIMTIME_MS), new cMessage("updateSchedule"));
        // scheduleAt(simTime() + SimTime(100, SIMTIME_MS), new cMessage("updateSchedule"));
    }

    NicForwardingApplication::initialize(stage);
}

void DynSchedForwardingApplication::handleMessageWhenUp(cMessage* msg)
{
    if (msg->isSelfMessage()) {
        if (std::string(msg->getName()).find("updateSchedule") != std::string::npos) {

            auto internalConfig = getSchedulerConfiguration();
            tsnScheduler->par("configuration").parse(internalConfig.c_str());
            // TODO: update interval
            auto now = simTime();
            auto next = now + SimTime(100, SIMTIME_MS);
            scheduleAt(simTime() + SimTime(100, SIMTIME_MS), new cMessage("updateSchedule"));
            return;
        }
    }
    NicForwardingApplication::handleMessageWhenUp(msg);
}

void DynSchedForwardingApplication::socketDataArrived(inet::UdpSocket* socket, inet::Packet* packet)
{
    // ignore local echoes
    auto srcAddr = packet->getTag<inet::L3AddressInd>()->getSrcAddress();
    if (srcAddr == inet::Ipv4Address::LOOPBACK_ADDRESS) {
        EV_DEBUG << "Ignored local echo: " << packet << endl;
        return;
    }

    // statistics
    emit(inet::packetReceivedSignal, packet);

    // try rescheduling the schedule
    adaptSchedule(packet);

    // process incoming packet
    processPacket(packet);
}

void DynSchedForwardingApplication::adaptSchedule(inet::Packet* packet)
{
    // we identify a stream by its source, destination and
    auto inboundInterface = packet->getTag<inet::InterfaceInd>()->getInterfaceId();
    inet::IInterfaceTable* interfaceTable = inet::getModuleFromPar<inet::IInterfaceTable>(par("interfaceTableModule"), this);
    inet::NetworkInterface* internalInterface = interfaceTable->findInterfaceByName("eth0");
    inet::NetworkInterface* externalInterface = interfaceTable->findInterfaceByName("wlan0");

    ASSERT2(internalInterface, "No internal interface with the name eth0 found");
    ASSERT2(externalInterface, "No external interface with the name wlan0 found");

    if (inboundInterface == internalInterface->getInterfaceId()) {
        // we don't care about internal packets
        return;
    }
    // TODO: If is not needed?
    else if (inboundInterface == externalInterface->getInterfaceId()) {
        // we get a new packt from the external interface
        auto l3AddressTagBase = packet->getTag<inet::L3AddressInd>();
        auto destAddr = l3AddressTagBase->getDestAddress();
        auto srcAddr = l3AddressTagBase->getSrcAddress();
        intval_t pcp = 0;
        std::string destination = "";
        for (auto i = 0; i < forwardingTable->size(); i++) {
            auto entry = forwardingTable->getEntry(i);
            if (std::string(packet->getName()).find(entry.first) != std::string::npos) {
                auto rule = check_and_cast<cValueMap*>(entry.second.objectValue());
                pcp = rule->get("pcp").intValue();
                destination = rule->get("destination").stringValue();
                break;
            }
        }

        // find the modules
        std::shared_ptr<TsnStream> correct_stream = nullptr;
        auto l3AddressResolver = inet::L3AddressResolver();
        for (auto stream : tsnStreams) {

            if (stream->getPCP() != pcp) {
                continue;
            }

            if (stream->getSourceModule() != this->getParentModule()) {
                continue;
            }

            // TODO: this is bullshit -> I have to check the applicaition and make the application in a way that I can identify it as the externel module. I still can set the external source but I should implicitly set the application in the case and check at this line of code only for the application because for a "normal" TSN stream this will try to resolve a nullpointer, sadge
            // But to be able to do so, I first need to use externalApplications as real applications
            auto externalModule = stream->getExternalSourceModule();
            // TODO: Check if realy correct
            if (externalModule == nullptr || l3AddressResolver.findHostWithAddress(srcAddr) != stream->getExternalSourceModule()) {
                continue;
            }

            if (stream->getDestinationModule()->getFullName() != destination) {
                continue;
            }

            // we found the stream
            correct_stream = stream;
            EV_DEBUG << "Found the correct stream: " << stream->getStreamName() << EV_ENDL;
            break;
        }

        if (correct_stream == nullptr) {
            EV_DEBUG << "No stream found for the packet. Registering a new one." << EV_ENDL;
            auto configLine = cValueMap();
            configLine.set("source", getParentModule()->getFullName());
            configLine.set("destination", destination);
            configLine.set("pcp", pcp);
            configLine.set("gateIndex", pcp);
            configLine.set("packetLength", cValue(packet->getTotalLength().get(), "b"));
            configLine.set("packetInterval", cValue(0, "s"));
            configLine.set("application", this->getFullName());

            auto newStream = std::make_shared<TsnStream>(new TsnStream(&configLine, this->getParentModule()->getParentModule(), l3AddressResolver.findHostWithAddress(srcAddr)));
            newStream->pushTime(simTime());
            newStream->setStartTime();
            tsnStreams.push_back(newStream);
            return;
        }

        // we found the correct stream and now we update it
        ASSERT2(correct_stream != nullptr, "This should never be reached");

        auto now = simTime();
        correct_stream->pushTime(now);

        auto newSlotSize = correct_stream->getNewSlotSize();

        // statistics yay
        auto signal_name = std::string("slot_size_") + std::string(correct_stream->getStreamName());
        simsignal_t signal = registerSignal(signal_name.c_str());
        cProperty* statisticTemplate = getProperties()->get("statisticTemplate", "slot_size");

        getEnvir()->addResultRecorders(this, signal, signal_name.c_str(), statisticTemplate);
        emit(signal, newSlotSize.get());

        correct_stream->setPacketSize(newSlotSize); // TODO: make this in a way that we check if we need to update
        correct_stream->setPacketInterval();
        // auto internalConfig = getSchedulerConfiguration();
        // EV_DEBUG << "New configuration: " << internalConfig << EV_ENDL;
        // tsnScheduler->par("configuration").parse(internalConfig.c_str());
    }
}

std::string DynSchedForwardingApplication::getSchedulerConfiguration()
{
    auto internalConfig = std::stringstream();
    internalConfig << "[";
    bool first = true;
    for (auto stream : tsnStreams) {
        // interval 0 means that we received the packet only once TODO
        // TODO: maybe extra value
        if (stream->getPacketInterval() == inet::ms(0)) {
            continue;
        }
        if (!first) {
            internalConfig << ", ";
        }
        internalConfig << *stream;
        first = false;
    }
    internalConfig << "]";
    return internalConfig.str();
}

void DynSchedForwardingApplication::testConfiguration()
{

    auto ogConfig = tsnScheduler->par("configuration").str();
    auto internalConfig = getSchedulerConfiguration();
    EV_DEBUG << "TEST DYNSCHED" << EV_ENDL;
    EV_DEBUG << ogConfig << EV_ENDL;
    EV_DEBUG << internalConfig << EV_ENDL;

    // check if the configurator is happy
    tsnScheduler->par("configuration").parse(internalConfig.c_str());

    auto stream = tsnStreams.at(0);
    for (auto i = 0; i < 10; i++) {
        stream->pushTime(SimTime(10, omnetpp::SIMTIME_S));
    }
    EV_DEBUG << "TEST DYNSCHED SLOT SIZE" << EV_ENDL;
    EV_DEBUG << stream->getNewSlotSize() << EV_ENDL;

    EV_DEBUG << "TEST DYNSCHED SLOT SIZE NOT THE SAME" << EV_ENDL;
    stream->pushTime(SimTime(1, omnetpp::SIMTIME_MS));
    EV_DEBUG << stream->getNewSlotSize() << EV_ENDL;

    stream->setPacketSize(stream->getNewSlotSize());
    internalConfig = getSchedulerConfiguration();
    tsnScheduler->par("configuration").parse(internalConfig.c_str());

    // TODO: Maybe a test to check if the slot size is actually the size I want to have? I don't know...

    // need to reset the configuration as well for the test, we will see...
}

simtime_t DynSchedForwardingApplication::getCreationTimeOffset()
{
    // TODO: hard coded cycle time
    auto diff_time = fmod(creationTime.trunc(omnetpp::SIMTIME_US), SimTime(100, omnetpp::SIMTIME_MS));
    auto tmp5 = diff_time.str();
    return diff_time;
}
} // namespace veins