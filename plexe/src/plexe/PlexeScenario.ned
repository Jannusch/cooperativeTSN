//
// Copyright (C) 2008 Christoph Sommer <christoph.sommer@informatik.uni-erlangen.de>
// Copyright (C) 2012-2023 Michele Segata <segata@ccs-labs.org>
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

package org.car2x.plexe;

import org.car2x.veins.base.connectionManager.ConnectionManager;
import org.car2x.veins.base.modules.BaseWorldUtility;
import org.car2x.veins.modules.world.annotations.AnnotationManager;

import org.car2x.plexe.PlexeManager;
import org.car2x.plexe.traci.PlexeScenarioManager;
import org.car2x.plexe.traci.PlexeScenarioManagerLaunchd;
import org.car2x.plexe.traci.PlexeScenarioManagerForker;
import org.car2x.plexe.mobility.TraCIBaseTrafficManager;
import org.car2x.plexe.PlatoonCar;

network PlexeScenario
{
    parameters:
        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)
        string traffic_type;
        bool useLaunchd = default(false);
        string manager_type = useLaunchd ? "PlexeScenarioManagerLaunchd" : "PlexeScenarioManagerForker";
        @display("bgb=dropUnit($playgroundSizeX),dropUnit($playgroundSizeY)");
    submodules:
        annotations: AnnotationManager {
            @display("p=260,50");
        }
        connectionManager: ConnectionManager {
            parameters:
                @display("p=150,0;i=abstract/multicast");
        }
        world: BaseWorldUtility {
            parameters:
                playgroundSizeX = veins_eval_by_version(veins_omnetpp_buildnum(), "playgroundSizeX", 1525, "parent.playgroundSizeX");
                playgroundSizeY = veins_eval_by_version(veins_omnetpp_buildnum(), "playgroundSizeY", 1525, "parent.playgroundSizeY");
                playgroundSizeZ = veins_eval_by_version(veins_omnetpp_buildnum(), "playgroundSizeZ", 1525, "parent.playgroundSizeZ");
                @display("p=30,0;i=misc/globe");
        }
        manager: <manager_type> like PlexeScenarioManager {
            parameters:
                @display("p=512,128");
        }
        plexe: PlexeManager {
            @display("p=280,50");
        }
        traffic: <traffic_type> like TraCIBaseTrafficManager {
            parameters:
                @display("p=200,200");
        }
        node[0]: PlatoonCar {
        }

    connections allowunconnected:
}

