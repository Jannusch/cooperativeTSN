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

package org.car2x.veins.subprojects.cooperative_tsn.applications;

import inet.applications.udpapp.UdpBasicApp;

simple SamplePlatoonApplication extends UdpBasicApp
{
    parameters:
        @class(veins::SamplePlatoonApplication);
        string finalAddress = default(""); // will not be forwarded by default

        @signal[end2endDelay](type="simtime_t");
        @statistic[end2endDelay](title="End-to-end delay"; source=end2endDelay; record=vector,stats);

}