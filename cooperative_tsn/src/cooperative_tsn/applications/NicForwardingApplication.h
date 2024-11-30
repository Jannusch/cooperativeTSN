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

#include "veins_inet/VeinsInetApplicationBase.h"

namespace veins {

class NicForwardingApplication : public VeinsInetApplicationBase {

private:
    omnetpp::simsignal_t forwardedToExternalSignal;
    omnetpp::simsignal_t forwardedToInternalSignal;
    int localPort;
    std::string multicastAddress;

protected:
    cValueMap* forwardingTable;

protected:
    virtual void initialize(int stage) override;
    void handleStartOperation(inet::LifecycleOperation* doneCallback) override;
    void processPacket(inet::Packet*);
    void socketDataArrived(inet::UdpSocket* socket, inet::Packet* packet) override;
};

} // namespace veins