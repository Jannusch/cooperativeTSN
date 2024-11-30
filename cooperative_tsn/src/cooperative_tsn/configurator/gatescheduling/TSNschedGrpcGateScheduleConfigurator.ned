//
// Copyright (C) 2024 Jannusch Bigge <jannusch.bigge@tu-dresden.de>
//
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
// This version is an adaptation of the original TSNschedGateScheduleConfigurator from INET which uses gRPC to communicate with the TSNsched tool.
//


package org.car2x.veins.subprojects.cooperative_tsn.configurator.gatescheduling;

import inet.linklayer.configurator.gatescheduling.common.TSNschedGateScheduleConfigurator;

simple TSNschedGrpcGateScheduleConfigurator extends TSNschedGateScheduleConfigurator 
{
    parameters:
        @class(inet::TSNschedGrpcGateScheduleConfigurator);
        string grpcAddress = default("172.17.0.3"); // network address of the scheduler (like dns name, IPv6, etc.)
        string grpcPort = default("50051"); // port of the gRPC server
        string pathToCient = default("../../src/gRPC/src/"); // Path to the Python gRPC client

}

