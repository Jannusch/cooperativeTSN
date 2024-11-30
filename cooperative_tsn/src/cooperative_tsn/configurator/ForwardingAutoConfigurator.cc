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

#include <vector>

#include "cooperative_tsn/configurator/ForwardingAutoConfigurator.h"

#include "inet/common/FindModule.h"
#include "inet/common/Topology.h"
#include "inet/linklayer/common/MacAddress.h"
#include "inet/linklayer/configurator/MacForwardingTableConfigurator.h"
#include "inet/linklayer/ethernet/common/MacForwardingTable.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/contract/IInterfaceTable.h"

namespace veins {
using namespace omnetpp;

Define_Module(ForwardingAutoConfigurator);

void ForwardingAutoConfigurator::initialize()
{
    getSimulation()->getSystemModule()->subscribe("registerNic", this);
    getSimulation()->getSystemModule()->subscribe("deleteNic", this);
}

void ForwardingAutoConfigurator::receiveSignal(cComponent* src, simsignal_t id, intval_t value, cObject* details)
{
    if (std::string(getSignalName(id)).find("registerNic") != std::string::npos) {
        registerNic(value);
    }
    else if (std::string(getSignalName(id)).find("deleteNic") != std::string::npos) {
        deleteNic(value);
    }
}

bool ForwardingAutoConfigurator::registerNic(intval_t value)
{

    inet::L3AddressResolver resolver;
    auto existing_nic_macs = std::vector<inet::MacAddress>();
    for (auto nic : registerdNics) {
        auto interfaceTable = resolver.findInterfaceTableOf(nic);
        for (int i = 0; i < interfaceTable->getNumInterfaces(); i++) {
            auto interface = interfaceTable->getInterface(i);
            if (!interface->isLoopback())
                existing_nic_macs.push_back(interface->getMacAddress());
        }
    }

    // calculate the multicast address
    ASSERT2(hasPar("multicastAddress"), "No multicast address specified. Needed to simulate broadcast in the wireless network.");
    auto mcastMac = inet::L3AddressResolver().resolve(par("multicastAddress")).mapToMulticastMacAddress();

    auto insertion_result = registerdNics.insert(getSimulation()->getModule(value));
    if (!insertion_result.second) {
        EV_INFO << "Module already registered\n";
        return false;
    }

    // new mac addresses
    std::vector<inet::MacAddress> newMacAddresses;
    inet::IInterfaceTable* interfaceTable = resolver.findInterfaceTableOf((cModule*) *insertion_result.first);
    ASSERT2(interfaceTable, "No interface table found for newly registered nic");
    for (int i = 0; i < interfaceTable->getNumInterfaces(); i++) {
        auto interface = interfaceTable->getInterface(i);
        if (!interface->isLoopback())
            newMacAddresses.push_back(interface->getMacAddress());
    }

    // iterate over all nics and configure forwarding tables for each bridge node in the subnetwork
    for (auto nic : registerdNics) {
        if (nic == *insertion_result.first) {
            // iterator points to the newly inserted element
            // configuration will be done in a seperate step as we do not want to include the new nic itself instead inserting all the other nics
            continue;
        }
        EV_INFO << "Configuring forwarding tables for " << nic->getParentModule() << "\n";
        auto node = nic->getParentModule();

        inet::FindModule<inet::MacForwardingTableConfigurator*> macForwardingTableConfiguratorFinder;
        auto forwardingConfigurator = macForwardingTableConfiguratorFinder.findSubModule(node);
        if (!forwardingConfigurator) {
            EV_ERROR << "No MacForwardingTableConfigurator found for node " << node->getFullPath() << "\n";
            continue;
        }

        // go through all nodes in the subnetwork and check if it is one to be configured
        inet::Topology* topology = forwardingConfigurator->getTopology();
        for (int j = 0; j < topology->getNumNodes(); j++) {
            auto sourceNode = (inet::Topology::Node*) topology->getNode(j);
            if (!isBridgeNode(sourceNode))
                continue;

            // check if switch
            inet::FindModule<inet::MacForwardingTable*> finder;
            auto macTable = finder.findSubModule(sourceNode->getModule());
            if (!macTable)
                continue;

            // add all mac addresses of the new nic to the forwarding table
            // all just in case the routing chooses the wrong interface as the dest interface
            // loop through all interfaces of the nic, as we do not know how many of them are connected
            auto interfaceTable = resolver.findInterfaceTableOf(nic);
            for (int i = 0; i < interfaceTable->getNumInterfaces(); i++) {
                auto interface = interfaceTable->getInterface(i);
                auto macAddress = interface->getMacAddress();
                auto result = macTable->getUnicastAddressForwardingInterface(macAddress);

                // add the multicast address for the wireless network
                // -1 is loopback CHECK
                if (macTable->getMulticastAddressForwardingInterfaces(mcastMac).empty() && result != -1)
                    macTable->addMulticastAddressForwardingInterface(result, mcastMac);

                // add the specific mac addresses of the new nics
                for (auto newMacAddress : newMacAddresses) {
                    macTable->setUnicastAddressForwardingInterface(result, newMacAddress);
                }
            }
        }
    }

    // check if there are other nics
    if (existing_nic_macs.empty()) {
        return true;
    }

    // add all the old mac addresses to the new nic
    auto nic = *insertion_result.first;
    EV_INFO << "Configuring forwarding tables for " << nic->getParentModule() << "\n";
    auto node = nic->getParentModule();

    inet::FindModule<inet::MacForwardingTableConfigurator*> macForwardingTableConfiguratorFinder;
    auto forwardingConfigurator = macForwardingTableConfiguratorFinder.findSubModule(node);
    if (!forwardingConfigurator) {
        EV_ERROR << "No MacForwardingTableConfigurator found for node " << node->getFullPath() << "\n";
        return false;
    }

    // go through all nodes in the subnetwork and check if it is one to be configured
    inet::Topology* topology = forwardingConfigurator->getTopology();
    for (int j = 0; j < topology->getNumNodes(); j++) {
        auto sourceNode = (inet::Topology::Node*) topology->getNode(j);
        if (!isBridgeNode(sourceNode))
            continue;

        // check if switch
        inet::FindModule<inet::MacForwardingTable*> finder;
        auto macTable = finder.findSubModule(sourceNode->getModule());
        if (!macTable)
            continue;

        // add all mac addresses of the existing nics to the forwarding table
        // all just in case the routing chooses the wrong interface as the dest interface
        // loop through all interfaces of the nic, as we do not know how many of them are connected
        auto interfaceTable = resolver.findInterfaceTableOf(nic);
        for (int i = 0; i < interfaceTable->getNumInterfaces(); i++) {
            auto interface = interfaceTable->getInterface(i);
            auto macAddress = interface->getMacAddress();
            auto result = macTable->getUnicastAddressForwardingInterface(macAddress);

            // add the multicast address for the wireless network
            // -1 is loopback CHECK
            if (macTable->getMulticastAddressForwardingInterfaces(mcastMac).empty() && result != -1)
                macTable->addMulticastAddressForwardingInterface(result, mcastMac);

            for (auto newMacAddress : existing_nic_macs) {
                macTable->setUnicastAddressForwardingInterface(result, newMacAddress);
            }
        }
    }
    return true;
}

bool ForwardingAutoConfigurator::deleteNic(intval_t value)
{
    return false;
}

// This is the same method as in inet::NetworkConfiguratorBase
// With an additional check for ipv6
bool ForwardingAutoConfigurator::isBridgeNode(inet::Topology::Node* node)
{
    inet::L3AddressResolver resolver; // for what ever reason thats not static
    auto interfaceTable = resolver.findInterfaceTableOf(node->getModule());
    auto ipv4routingTable = resolver.findIpv4RoutingTableOf(node->getModule());
    auto ipv6routingTable = resolver.findIpv6RoutingTableOf(node->getModule());
    return interfaceTable || ipv4routingTable || ipv6routingTable;
}
} // namespace veins