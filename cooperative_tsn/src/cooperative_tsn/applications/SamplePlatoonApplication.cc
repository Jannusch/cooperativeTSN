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
// Application based on the UdpApp to send and receive messages with custom message types
//

#include "cooperative_tsn/applications/SamplePlatoonApplication.h"
#include "cooperative_tsn/applications/Messages/BaseForwardingMessage_m.h"

#include "inet/common/Simsignals.h"
#include "inet/common/TimeTag_m.h"
#include "inet/networklayer/common/FragmentationTag_m.h"

namespace veins {
using namespace inet;
Define_Module(SamplePlatoonApplication);

void SamplePlatoonApplication::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        end2endDelaySignal = registerSignal("end2endDelay");
    }
    UdpBasicApp::initialize(stage);
}

void SamplePlatoonApplication::sendPacket()
{
    std::ostringstream str;
    str << "SamplePlatoon"
        << "-" << numSent;
    Packet* packet = new Packet(str.str().c_str());
    if (dontFragment)
        packet->addTag<FragmentationReq>()->setDontFragment(true);
    // Using SimplePlatoon message type instead of ApplicationPacket
    const auto& payload = inet::makeShared<BaseForwardingMessage>();
    payload->setChunkLength(B(par("messageLength")));
    payload->setCarId(std::string("Car " + std::to_string(getParentModule()->getIndex())).c_str());
    payload->setAddress(par("finalAddress").stringValue());
    payload->addTag<CreationTimeTag>()->setCreationTime(simTime());
    packet->insertAtBack(payload);
    L3Address destAddr = chooseDestAddr();
    emit(packetSentSignal, packet);
    socket.sendTo(packet, destAddr, destPort);
    numSent++;
}

void SamplePlatoonApplication::handleMessageWhenUp(cMessage* msg)
{
    auto packet = dynamic_cast<Packet*>(msg);
    if (packet) {
        auto payload = packet->peekAtFront<BaseForwardingMessage>();
        if (payload) {
            auto creationTime = payload->getTag<CreationTimeTag>()->getCreationTime();
            emit(end2endDelaySignal, simTime() - creationTime);
        }
    }
    UdpBasicApp::handleMessageWhenUp(msg);
}

} // namespace veins