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

package org.car2x.veins.subprojects.cooperative_tsn.nodes;

import org.car2x.veins.base.modules.IBaseApplLayer;
import org.car2x.veins.modules.mobility.traci.TraCIMobility;
import org.car2x.veins.modules.nic.Nic80211p;

import org.car2x.plexe.utilities.BasePositionHelper;
import org.car2x.plexe.scenarios.BaseScenario;
import org.car2x.plexe.protocols.BaseProtocol;
import org.car2x.plexe.apps.BaseApp;
import org.car2x.plexe.driver.Veins11pRadioDriver;


////////////////////////////////////
import org.car2x.veins.nodes.Car;

import org.car2x.veins.subprojects.cooperative_tsn.nodes.TsnNic;
// TSN from INET
import inet.node.ethernet.EthernetApp;
import inet.node.tsn.TsnClock;
import inet.node.tsn.TsnDevice;
import inet.node.tsn.TsnSwitch;

import inet.node.ethernet.EthernetLink;

import inet.networklayer.configurator.ipv4.HostAutoConfigurator;

import inet.networklayer.configurator.contract.INetworkConfigurator;
import inet.linklayer.configurator.gatescheduling.contract.IGateScheduleConfigurator;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.linklayer.configurator.gatescheduling.common.TSNschedGateScheduleConfigurator;

import inet.visualizer.common.GateScheduleVisualizer;

import inet.node.inet.AdhocHost;

import inet.physicallayer.wireless.unitdisk;
import inet.physicallayer.wireless.*;
import org.car2x.veins.modules.mobility.traci.TraCIMobility;
import org.car2x.veins.subprojects.veins_inet.VeinsInetMobility;
import inet.linklayer.contract.IEthernetInterface;
import inet.applications.contract.IApp;

import org.car2x.veins.subprojects.veins_inet.VeinsInetTransparentMobility;

import org.car2x.veins.subprojects.cooperative_tsn.nodes.PlatoonDevice;
import org.car2x.veins.subprojects.cooperative_tsn.applications.DynSchedForwardingApplicaion;
import org.car2x.veins.subprojects.cooperative_tsn.plexe_driver.InetDriver;

/////////////////////
module PlatoonTSNCar
{
    parameters:
        @display("p=50,50");
        string scenario_type;
        string helper_type;
        string appl_type;
        string protocol_type;

        **.hasGlobalArp = true;

        **.defaultMulticastLoop = false;

    submodules:

        helper: <helper_type> like BasePositionHelper {
            parameters:
                @display("p=871,482");
        }

        scenario: <scenario_type> like BaseScenario {
            parameters:
                @display("p=569,713");
        }

        appl: <appl_type> like BaseApp {
            parameters:
                @display("p=871,368");
        }

        prot: <protocol_type> like BaseProtocol {
            parameters:
                @display("p=755,368");
        }

        mobility: TraCIMobility {
            parameters:
                @display("p=659,713;i=block/cogwheel");
        }

        inetDriver: InetDriver {
            @display("p=755,482,row");
            destinationPort = 3000;
            multicastAddress = "224.0.0.1";
        }

        gateScheduleConfigurator: <default("")> like IGateScheduleConfigurator if typename != "" {
            @display("p=367,641");
        }
        // TSN subnetwork
        tsnClock: TsnClock {

            @display("p=475,36");
        }

        tsnSwitch[1]: TsnSwitch {
            @display("p=328,172,ri,120,120");
        }

        // tsnDevice[4]: TsnDevice {

        //     @display("p=210,482,r,50");
        // }

        tsnNic: TsnNic;

        platoonDevice: PlatoonDevice {
            @display("p=608,482");
        }

        macForwardingTableConfigurator: <default("MacForwardingTableConfigurator")> like INetworkConfigurator if typename != "" {
            @display("p=367,713;is=s");
        }

        streamRedundancyConfigurator: <default("")> like INetworkConfigurator if typename != "" {
            @display("p=769,641;is=s");
        }
        failureProtectionConfigurator: <default("")> like INetworkConfigurator if typename != "" {
            @display("p=569,641;is=s");
        }

        inet_mobility: VeinsInetTransparentMobility {
            @display("p=739,713");
            mobilityModule = default("^.mobility");
        }
    connections allowunconnected:
        // tsnClock.ethg++ <--> EthernetLink <--> tsnSwitch.ethg++;
        // tsnSwitch.ethg++ <--> EthernetLink <--> tsnDevice[1].ethg++;
        // tsnSwitch[1].ethg++ <--> EthernetLink <--> tsnDevice[0].ethg++;
        // tsnSwitch[1].ethg++ <--> EthernetLink <--> tsnDevice[1].ethg++;
        // tsnSwitch[2].ethg++ <--> EthernetLink <--> tsnDevice[2].ethg++;
        // tsnSwitch[2].ethg++ <--> EthernetLink <--> tsnDevice[3].ethg++;

        tsnSwitch[0].ethg++ <--> EthernetLink <--> tsnClock.ethg++;
        tsnSwitch[0].ethg++ <--> EthernetLink <--> tsnNic.ethg++;

        // tsnSwitch[0].ethg++ <--> EthernetLink <--> tsnSwitch[1].ethg++;
        // tsnSwitch[1].ethg++ <--> EthernetLink <--> tsnSwitch[2].ethg++;
        tsnSwitch[0].ethg++ <--> EthernetLink <--> platoonDevice.ethg++;
        // redundant link
        // tsnSwitch[0].ethg++ <--> EthernetLink <--> tsnSwitch[2].ethg++;

        inetDriver.upperLayerIn <-- prot.radiosOut++;
        inetDriver.upperLayerOut --> prot.radiosIn++;
        inetDriver.socketOut --> platoonDevice.in++;
        inetDriver.socketIn <-- platoonDevice.out++;


}
