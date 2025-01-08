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

package org.car2x.veins.subprojects.cooperative_tsn.applications;

import org.car2x.veins.subprojects.veins_inet.VeinsInetApplicationBase;

simple NicForwardingApplication extends VeinsInetApplicationBase
{
    parameters:
        @class(veins::NicForwardingApplication);
        object forwardingRules; // rules which message to forward to which module (relative path from the parent of the internal network or absolut path with usual ned notation) (e.g. {"SamplePlatoon": {"destination": "tsnDevice", "port": 3000, "pcp": 0}})
        int localPort = default(3000); // UDP port to listen on
        string multicastAddress = default("224.0.0.1"); // multicast address to listen on for wireless broadcast
        
        // statistics
        @signal[forwardedToExternalNetwork](type="int");
        @statistic[forwardedToExternalNetwork](title="Number forwarded packets to external network"; source=forwardedToExternalNetwork; record=vector,count);

        @signal[forwardedToInternalNetwork](type="int");
        @statistic[forwardedToInternalNetwork](title="Number forwarded packets to internal network"; source=forwardedToInternalNetwork; record=vector,count);
}
