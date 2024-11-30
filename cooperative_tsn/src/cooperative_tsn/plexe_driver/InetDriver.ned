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

package org.car2x.veins.subprojects.cooperative_tsn.plexe_driver;


import inet.applications.contract.IApp;

simple InetDriver like IApp
{
    parameters:
        @class(cooperative_tsn::InetDriver);
        int destinationPort = default(3000);
        string multicastAddress = default("224.0.0.1");
        string interfaceTableModule = default("^.platoonDevice.interfaceTable");

        @display("i=block/ifcard");

        // statisitcs
        // @signal[end2endDelay](type="simtime_t");
        // @statistic[end2endDelay](title="End-to-end delay"; unit=doube; source=end2endDelay; record=vector,stats);

        @signal[e2e_from_*](type="simtime_t");
        @statisticTemplate[end2endDelay](record=vector,stats);
    gates:
        output socketOut;
        input socketIn;
        output upperLayerOut;
        input upperLayerIn;
}
