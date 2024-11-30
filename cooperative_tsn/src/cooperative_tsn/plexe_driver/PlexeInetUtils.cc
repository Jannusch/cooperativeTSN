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

#include <deque>

#include "cooperative_tsn/plexe_driver/PlexeInetUtils.h"

#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/chunk/Chunk.h"
#include "inet/common/packet/chunk/cPacketChunk.h"
#include "inet/common/packet/chunk/SequenceChunk.h"
// #include "plexe_lte/messages/PlatoonUpdateMessage_m.h"

namespace cooperative_tsn {

PlexeInetUtils::PlexeInetUtils()
{
}

PlexeInetUtils::~PlexeInetUtils()
{
}

inet::Packet* PlexeInetUtils::encapsulate(omnetpp::cPacket* packet, const char* name)
{
    inet::Packet* container = new inet::Packet(name);
    auto data = inet::makeShared<inet::cPacketChunk>(packet);
    auto timeTag = data->addTag<inet::CreationTimeTag>();
    timeTag->setCreationTime(omnetpp::simTime());
    container->insertAtBack(data);
    return container;
}
const inet::cPacketChunk* PlexeInetUtils::decapsulate(inet::Packet* packet)
{
    const inet::cPacketChunk* pc;

    pc = dynamic_cast<const inet::cPacketChunk*>(packet->peekAll().get());

    if (pc)
        return pc;

    const inet::SequenceChunk* data = dynamic_cast<const inet::SequenceChunk*>(packet->peekAll().get());
    if (data) {
        const std::deque<inet::Ptr<const inet::Chunk>> chunks = data->getChunks();

        // TODO check why switch to -2 instead of -1
        pc = dynamic_cast<const inet::cPacketChunk*>(chunks[chunks.size() - 2].get());
        if (pc)
            return pc;
        else
            return nullptr;
    }

    return nullptr;
}

} // namespace cooperative_tsn
