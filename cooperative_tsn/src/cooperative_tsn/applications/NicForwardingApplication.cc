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
#include <cassert>
#include <string>

#include "cooperative_tsn/applications/NicForwardingApplication.h"

#include "inet/common/ModuleAccess.h"
#include "inet/common/Ptr.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/common/packet/Packet.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/common/NetworkInterface.h"
#include "inet/networklayer/contract/IInterfaceTable.h"

#include "veins_inet/VeinsInetApplicationBase.h"

namespace veins {
Define_Module(NicForwardingApplication);

void NicForwardingApplication::initialize(int stage)
{

    if (stage == inet::INITSTAGE_LOCAL) {
        // parse forwarding table
        forwardingTable = check_and_cast<cValueMap*>(par("forwardingRules").objectValue());

        // create signals for statistics
        forwardedToExternalSignal = registerSignal("forwardedToExternalNetwork");
        forwardedToInternalSignal = registerSignal("forwardedToInternalNetwork");

        localPort = par("localPort").intValue();
        multicastAddress = par("multicastAddress").stringValue();
    }
    VeinsInetApplicationBase::initialize(stage);
}

void NicForwardingApplication::handleStartOperation(inet::LifecycleOperation* operation)
{
    socket.setOutputGate(gate("socketOut"));
    // Todo: finde the solution how to bind to all ports
    socket.bind(inet::L3Address(), localPort);

    inet::IInterfaceTable* interfaceTable = inet::getModuleFromPar<inet::IInterfaceTable>(par("interfaceTableModule"), this);
    inet::NetworkInterface* internalInterface = interfaceTable->findInterfaceByName("eth0");
    inet::NetworkInterface* externalInterface = interfaceTable->findInterfaceByName("wlan0");

    ASSERT2(internalInterface, "No internal interface with the name eth0 found");
    ASSERT2(externalInterface, "No external interface with the name wlan0 found");

    socket.setMulticastOutputInterface(externalInterface->getInterfaceId());
    inet::MulticastGroupList mgl = interfaceTable->collectMulticastGroups();
    socket.joinLocalMulticastGroups(mgl);
    socket.setMulticastLoop(false);

    socket.setCallback(this);
    ASSERT(startApplication());
}

void NicForwardingApplication::processPacket(inet::Packet* packet)
{
    // first check interface and than decide where to send the packet

    auto payload = packet->peekAtFront();
    auto name = packet->getName();
    auto inboundInterface = packet->getTag<inet::InterfaceInd>()->getInterfaceId();
    auto srcAddress = packet->getTag<inet::L3AddressInd>()->getSrcAddress();

    inet::IInterfaceTable* interfaceTable = inet::getModuleFromPar<inet::IInterfaceTable>(par("interfaceTableModule"), this);
    inet::NetworkInterface* internalInterface = interfaceTable->findInterfaceByName("eth0");
    inet::NetworkInterface* externalInterface = interfaceTable->findInterfaceByName("wlan0");

    ASSERT2(internalInterface, "No internal interface with the name eth0 found");
    ASSERT2(externalInterface, "No external interface with the name wlan0 found");

    if (inboundInterface == internalInterface->getInterfaceId()) {
        // Create a new packet and send it to the multicast group via the wlan0 interface
        inet::Packet* forward_packet = new inet::Packet(name);
        // auto data = inet::makeShared<inet::cPacketChunk>(payload);
        forward_packet->insertAtBack(payload);

        // I'm not to 100% sure if this is needed
        // If the wlan0 is the only one in the multicas group and I send it to the multicast group
        // it should be send to the wlan0 interface
        forward_packet->addTagIfAbsent<inet::InterfaceReq>();
        auto interfaceReq = forward_packet->findTagForUpdate<inet::InterfaceReq>();
        interfaceReq->setInterfaceId(
            inet::getModuleFromPar<inet::IInterfaceTable>(
                par("interfaceTableModule"), this)
            ->findInterfaceByName("wlan0")
            ->getInterfaceId());

        // IP Layer
        // Try to convert to something that was as base at some time a BaseForwardingMessage
        // If not just use broadcast as default
        inet::L3Address destAddr = inet::L3AddressResolver().resolve(multicastAddress.c_str());
        forward_packet->addTagIfAbsent<inet::L3AddressReq>();
        auto l3tag = forward_packet->findTagForUpdate<inet::L3AddressReq>();
        l3tag->setDestAddress(destAddr);
        l3tag->setSrcAddress(srcAddress);
        l3tag->setNonLocalSrcAddress(true);

        emit(forwardedToExternalSignal, forward_packet->getByteLength());
        // TODO dest port
        socket.sendTo(forward_packet, destAddr, 3000);
    }
    else if (inboundInterface == externalInterface->getInterfaceId()) {
        // Check for the destination module in the forwarding table
        std::string destinationModule = "";
        int destinationPort;
        for (auto i = 0; i < forwardingTable->size(); i++) {
            auto entry = forwardingTable->getEntry(i);
            auto name_entry = entry.first;
            auto value_entry = check_and_cast<cValueMap*>(entry.second.objectValue());
            if (std::string(name).find(entry.first) != std::string::npos) {
                auto parentModule = getParentModule()->getParentModule()->getFullPath();
                destinationModule = parentModule + "." + value_entry->get("destination").stringValue();
                destinationPort = value_entry->get("port").intValue();
                break;
            }
        }
        ASSERT2(destinationModule.compare(""), "No destination module found in the forwarding table");
        // Create new packet with old payload and send it to the destination module
        inet::Packet* forward_packet = new inet::Packet(name);
        // auto data = inet::makeShared<inet::cPacketChunk>(payload);
        forward_packet->insertAtBack(payload);
        // IP Layer
        inet::L3Address destAddr = inet::L3AddressResolver().resolve(destinationModule.c_str());
        forward_packet->addTagIfAbsent<inet::L3AddressReq>();
        auto l3tag = forward_packet->findTagForUpdate<inet::L3AddressReq>();
        l3tag->setDestAddress(destAddr);
        l3tag->setSrcAddress(srcAddress);
        l3tag->setNonLocalSrcAddress(true);

        auto interfaceReq = forward_packet->addTagIfAbsent<inet::InterfaceReq>();
        interfaceReq->setInterfaceId(
            inet::getModuleFromPar<inet::IInterfaceTable>(
                par("interfaceTableModule"), this)
            ->findInterfaceByName("eth0")
            ->getInterfaceId());
        emit(forwardedToInternalSignal, forward_packet->getByteLength());
        socket.sendTo(forward_packet, destAddr, destinationPort);
    }
    else if (inboundInterface == interfaceTable->findInterfaceByName("lo")->getInterfaceId()) {
        // This is the loopback interface
    }
}

void NicForwardingApplication::socketDataArrived(inet::UdpSocket* socket, inet::Packet* packet)
{
    // ignore local echoes
    auto srcAddr = packet->getTag<inet::L3AddressInd>()->getSrcAddress();
    if (srcAddr == inet::Ipv4Address::LOOPBACK_ADDRESS) {
        EV_DEBUG << "Ignored local echo: " << packet << endl;
        return;
    }

    // statistics
    emit(inet::packetReceivedSignal, packet);

    // process incoming packet
    processPacket(packet);
}
} // namespace veins