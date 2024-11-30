//
// Copyright (C) 2008 Christoph Sommer <christoph.sommer@informatik.uni-erlangen.de>
// Copyright (C) 2020 Piermaria Arvani <piermaria.arvani@gmail.com>
//
// Documentation for these modules is at http://veins.car2x.org/
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

package org.car2x.veins.subprojects.cooperative_tsn.nodes;

import inet.node.tsn.TsnDevice;
import org.car2x.plexe.scenarios.BaseScenario;
import org.car2x.plexe.protocols.BaseProtocol;
import org.car2x.plexe.apps.BaseApp;
import org.car2x.veins.modules.mobility.traci.TraCIMobility;
import org.car2x.plexe.utilities.BasePositionHelper;


import org.car2x.veins.subprojects.cooperative_tsn.plexe_driver.InetDriver;
import org.car2x.veins.subprojects.cooperative_tsn.plexe_driver.BaseInetPositionHelper;



module PlatoonDevice extends TsnDevice
{
    parameters:
        @display("p=90,100;bgb=403,530");
    
        
    gates:
        input in[];    
        output out[];

    connections allowunconnected:
        in++ --> at.in++;
        out++ <-- at.out++;

}
