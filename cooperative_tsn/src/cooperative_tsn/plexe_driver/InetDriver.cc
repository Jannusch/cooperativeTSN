//
// Copyright (C) 2024 Jannusch Bigge <jannusch.bigge@tu-dresden.de>
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
// Adapted from LTECV2XMode3RadioDriver from Plexe_LTE
//

#include "cooperative_tsn/plexe_driver/InetDriver.h"
#include "cooperative_tsn/plexe_driver/PlexeInetUtils.h"

#include "inet/common/ModuleAccess.h"
#include "inet/common/TimeTag_m.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/common/L3AddressTag_m.h"

#include "omnetpp/clistener.h"
#include "omnetpp/cproperty.h"
#include "omnetpp/csimulation.h"

#include "veins/modules/messages/BaseFrame1609_4_m.h"

using namespace veins;

namespace cooperative_tsn {

Define_Module(InetDriver);

InetDriver::InetDriver()
    : destinationPort(3000)
    , socketInGate(-1)
    , socketOutGate(-1)
    , upperLayerIn(-1)
    , upperLayerOut(-1)
{
}

InetDriver::~InetDriver()
{
}

void InetDriver::initialize(int stage)
{

    cSimpleModule::initialize(stage);

    socketInGate = findGate("socketIn");
    socketOutGate = findGate("socketOut");
    upperLayerIn = findGate("upperLayerIn");
    upperLayerOut = findGate("upperLayerOut");

    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;

    end2endDelaySignal = registerSignal("end2endDelay");

    destinationPort = par("destinationPort");
    socket.setOutputGate(gate("socketOut"));
    socket.bind(destinationPort);
    // we want explicit addressing
    // setMulticastAddress(par("multicastAddress").stdstringValue());
}

void InetDriver::setMulticastAddress(std::string address)
{
    if (!multicastAddress.isUnspecified()) {
        // we are already bound to a multicast address. leave this group first
        socket.leaveMulticastGroup(multicastAddress);
    }
    multicastAddress = inet::L3AddressResolver().resolve(address.c_str());
    inet::IInterfaceTable* ift = inet::getModuleFromPar<inet::IInterfaceTable>(par("interfaceTableModule"), this);
    inet::NetworkInterface* ie = ift->findInterfaceByName("cellular");
    if (!ie)
        throw cRuntimeError("Wrong multicastInterface setting: no interface named wlan");
    socket.setMulticastOutputInterface(ie->getInterfaceId());
    socket.joinMulticastGroup(multicastAddress, ie->getInterfaceId());
}

void InetDriver::handleMessage(cMessage* msg)
{
    if (msg->getArrivalGateId() == upperLayerIn) {
        // TODO:
        // - [ ] add source address
        BaseFrame1609_4* frame = check_and_cast<BaseFrame1609_4*>(msg);
        auto destination = frame->getRecipientAddress();
        inet::L3Address destAddr;
        // we now know that this is an
        if (destination == -1)
            destAddr = inet::L3AddressResolver().resolve("224.0.0.1");
        else
            // TODO Address extraction
            destAddr = inet::L3AddressResolver().resolve("Platooning.node[0].platoonDevice");

        inet::Packet* container = PlexeInetUtils::encapsulate(frame, "BaseFrame1609_4_Container");
        // Add time tag for end2end delay to the base frame
        auto regionTimeTag = container->addRegionTagIfAbsent<inet::CreationTimeTag>();
        regionTimeTag->setCreationTime(simTime());

        socket.sendTo(container, destAddr, destinationPort);
    }
    else if (msg->getArrivalGateId() == socketInGate) {
        inet::Packet* container = check_and_cast<inet::Packet*>(msg);
        auto chunk = PlexeInetUtils::decapsulate(container);
        BaseFrame1609_4* frame = check_and_cast<BaseFrame1609_4*>(chunk->getPacket()->dup());
        auto regionTimeTag = chunk->getTag<inet::CreationTimeTag>();
        auto e2eDelay = simTime() - regionTimeTag->getCreationTime();

        // I need to build the name out of the module name from the sender and the receiver
        auto resolver = inet::L3AddressResolver();
        auto sender = container->getTag<inet::L3AddressInd>()->getSrcAddress();
        auto sender_module = resolver.findHostWithAddress(sender)->getParentModule()->getFullName();
        auto signalName = std::string("e2e_from_") + sender_module;

        simsignal_t signal = registerSignal(signalName.c_str());
        cProperty* statisticTemplate = getProperties()->get("statisticTemplate", "end2endDelay");
        getEnvir()->addResultRecorders(this, signal, signalName.c_str(), statisticTemplate);

        emit(signal, e2eDelay);

        send(frame, upperLayerOut);
        delete container;
    }
}

} // namespace cooperative_tsn
