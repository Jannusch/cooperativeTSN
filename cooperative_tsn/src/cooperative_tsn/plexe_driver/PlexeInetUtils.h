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

#pragma once

#include "inet/common/packet/chunk/cPacketChunk.h"
#include "inet/common/packet/Packet.h"

#include "omnetpp/cpacket.h"

namespace cooperative_tsn {
/**
 * @brief This class is a patched version of the PlexeInetUtils (yes I did not even changed the name...)
 * The main reason for the copy is, that the original Class depends on plexe_lte what itself requires SimuLTE.
 * Adding this dependency chain only for a small wrapper is to much and thats why this class is where it is.
 * In the patched version we also added a timestamp for easy statistics about the end-to-end delay and changed the return type to a inet::Packt from a inet::PacketChunk.
 */
class PlexeInetUtils {
public:
    PlexeInetUtils();
    virtual ~PlexeInetUtils();

    /**
     * Encapsulates a cPacket into an inet::Packet to pass Veins messages through INET
     */
    static inet::Packet* encapsulate(omnetpp::cPacket* packet, const char* name = "");
    /**
     * Iterates through INET Packet and decapsulates the second last packet which should be the Veins message.
     */
    static const inet::cPacketChunk* decapsulate(inet::Packet* packet);
};

} /* namespace cooperative_tsn */
