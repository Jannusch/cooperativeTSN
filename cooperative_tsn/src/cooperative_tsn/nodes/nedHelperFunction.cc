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

#include "omnetpp.h"

using namespace omnetpp;

/*
   Checks if external interfaces are activated and returns a list of interface names for the host auto configurator
   @param bool hasWLAN
   @param bool hasVeins
   @param bool hasspace_veins
   @param bool hasSimu5G
   @param ...
 */
static cNedValue returnInterfaces(cComponent* context, cNedValue argv[], int argc)
{
    std::string interfaces = "";
    for (int i = 0; i < 4; i++) {
        if (argv[i].getType() != cNedValue::BOOL)
            throw cRuntimeError("Invalid argument type, expected bool");
        if (argv[i].boolValue()) {
            // TODO: I need new names for the interfaces :(
            switch (i) {
            case 0:
                interfaces += "wlan0 ";
                break;
            case 1:
                interfaces += "wlan0 ";
                break;
            case 2:
                interfaces += "space_veins ";
                break;
            case 3:
                interfaces += "simu5G ";
                break;
            default:
                break;
            }
        }
    }
    // TODO: add option for additional interfaces

    return interfaces;
}

Define_NED_Function(returnInterfaces, "string returnInterfaces(bool hasWLAN, bool hasVeins, bool hasspace_veins, bool hasSimu5G, ...)")