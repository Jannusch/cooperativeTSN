//
// Copyright (C) 2013 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "inet/networklayer/configurator/base/NetworkConfiguratorBase.h"

#include <queue>
#include <set>
#include <vector>

#include "inet/common/ModuleAccess.h"
#include <vector>

#include "inet/common/ModuleAccess.h"
#include "inet/common/Topology.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/ipv4/IIpv4RoutingTable.h"

namespace inet {

void NetworkConfiguratorBase::extractTopology(Topology& topology)
{
    // We create two topologies and one with all modules under the parent module of the calling configurator and one with all network nodes
    // From this two topologies the cut is calculated and than be used as the final topology
    auto parentPath = getParentModule()->getFullPath();

    Topology *topologyByPath = new Topology();
    topologyByPath->extractByModulePath(cStringTokenizer((parentPath + ".*").c_str()).asVector());

    topology.extractByProperty("networkNode");

    // calculate cut of both topologies
    auto numberNodesInNetworkNode = topology.getNumNodes();
    std::vector<Node *> nodesToDelete;
    for (int i = 0; i < numberNodesInNetworkNode; i++) {
        auto nodeNetworkNode = (Node *)topology.getNode(i);
        
        for (int j = 0; j < topologyByPath->getNumNodes(); j++) {
            auto nodeModulePath = (Node *)topologyByPath->getNode(j);
            if (nodeNetworkNode->getModule() == nodeModulePath->getModule()) {
                goto skipInnerLoop;
            }
        }
        nodesToDelete.push_back(nodeNetworkNode);
// label to break two loops at once (sorry)
skipInnerLoop:  
        // Statement can be removed as soon as OMNeT++ is compatible with C++23 and lable without statement is allowed
        EV_DEBUG << "Node " << nodeNetworkNode->getModule()->getFullName() << " is in networkNode\n";
    }

    // for (int i = nodesToDelete.size() - 1; i >= 0; i--) {
    //     topology.deleteNode(topology.getNode(i));
    // }

    for(auto node: nodesToDelete){
        topology.deleteNode(node);
    }

    


    // END of custom topologie extraction

    
    EV_DEBUG << "Topology found " << topology.getNumNodes() << " nodes\n";
    if (topology.getNumNodes() == 0)
        throw cRuntimeError("Empty network!");
    // extract nodes, fill in interfaceTable and routingTable members in node
    L3AddressResolver addressResolver;
    for (int i = 0; i < topology.getNumNodes(); i++) {
        Node *node = (Node *)topology.getNode(i);
        node->module = node->getModule();
        node->interfaceTable = addressResolver.findInterfaceTableOf(node->module);
        node->routingTable = addressResolver.findIpv4RoutingTableOf(node->module);
    }
    // extract links and interfaces
    std::set<NetworkInterface *> interfacesSeen;
    std::queue<Node *> unvisited; // unvisited nodes in the graph
    auto rootNode = (Node *)topology.getNode(0);
    unvisited.push(rootNode);
    while (!unvisited.empty()) {
        Node *node = unvisited.front();
        unvisited.pop();
        IInterfaceTable *interfaceTable = node->interfaceTable;
        if (interfaceTable) {
            // push neighbors to the queue
            for (int i = 0; i < interfaceTable->getNumInterfaces(); i++) {
                NetworkInterface *networkInterface = interfaceTable->getInterface(i);
                if (interfacesSeen.count(networkInterface) == 0) {
                    // visiting this interface
                    interfacesSeen.insert(networkInterface);
                    Topology::Link *linkOut = findLinkOut(node, networkInterface->getNodeOutputGateId());
                    Node *childNode = nullptr;
                    if (linkOut) {
                        childNode = (Node *)linkOut->getLinkOutRemoteNode();
                        unvisited.push(childNode);
                    }
                    Interface *interface = new Interface(node, networkInterface);
                    node->interfaces.push_back(interface);
                }
            }
        }
    }

    // Now we have to add the ethernet nic to the topology and the connection
    // auto extIntNode = getParentModule()->getSubmodule("eth_inner");
    // auto gateName = extIntNode->getGateNames();
    // auto ethGate = extIntNode->gate("phys$o");
    // auto ethGateSwitch = ethGate->getNextGate();
    // Topology::Node* externNode = new Topology::Node(extIntNode->getParentModule()->getId());
    // Topology::Node* tsnSwitch = new Topology::Node(ethGateSwitch->getOwnerModule()->getId());
    // Topology::Link* externLink = new Topology::Link();
    // topology.addNode(externNode);
    // topology.addLink(externLink, tsnSwitch, externNode);
    // topology.addLink(externLink, externNode, tsnSwitch);

    // create Interface for the external Node
    // auto interfaces = addressResolver.findInterfaceTableOf(extIntNode->getParentModule());
    

    // annotate links with interfaces
    for (int i = 0; i < topology.getNumNodes(); i++) {
        Node *node = (Node *)topology.getNode(i);
        for (int j = 0; j < node->getNumOutLinks(); j++) {
            Topology::Link *linkOut = node->getLinkOut(j);
            Link *link = (Link *)linkOut;
            Node *localNode = (Node *)linkOut->getLinkOutLocalNode();
            if (localNode->interfaceTable)
                link->sourceInterface = findInterface(localNode, localNode->interfaceTable->findInterfaceByNodeOutputGateId(linkOut->getLinkOutLocalGateId()));
            Node *remoteNode = (Node *)linkOut->getLinkOutRemoteNode();
            if (remoteNode->interfaceTable)
                link->destinationInterface = findInterface(remoteNode, remoteNode->interfaceTable->findInterfaceByNodeInputGateId(linkOut->getLinkOutRemoteGateId()));
        }
    }

    // just add the values to the node and the link manually
}

std::vector<NetworkConfiguratorBase::Node *> NetworkConfiguratorBase::computeShortestNodePath(Node *source, Node *destination) const
{
    std::vector<Node *> path;
    topology->calculateUnweightedSingleShortestPathsTo(destination);
    auto node = source;
    while (node != destination) {
        path.push_back(node);
        node = (Node *)node->getPath(0)->getLinkOutRemoteNode();
    }
    path.push_back(destination);
    return path;
}

std::vector<NetworkConfiguratorBase::Link *> NetworkConfiguratorBase::computeShortestLinkPath(Node *source, Node *destination) const
{
    std::vector<Link *> path;
    topology->calculateUnweightedSingleShortestPathsTo(destination);
    auto node = source;
    while (node != destination) {
        auto link = (Link *)node->getPath(0);
        path.push_back(link);
        node = (Node *)node->getPath(0)->getLinkOutRemoteNode();
    }
    return path;
}

bool NetworkConfiguratorBase::isBridgeNode(Node *node) const
{
    return !node->routingTable || !node->interfaceTable;
}

NetworkConfiguratorBase::Link *NetworkConfiguratorBase::findLinkIn(const Node *node, const char *neighbor) const
{
    for (int i = 0; i < node->getNumInLinks(); i++)
        if (!strcmp(node->getLinkIn(i)->getLinkInRemoteNode()->getModule()->getFullName(), neighbor))
            return check_and_cast<Link *>(static_cast<Topology::Link *>(node->getLinkIn(i)));
    return nullptr;
}

NetworkConfiguratorBase::Link *NetworkConfiguratorBase::findLinkOut(const Node *node, const char *neighbor) const
{
    for (int i = 0; i < node->getNumOutLinks(); i++)
        if (!strcmp(node->getLinkOut(i)->getLinkOutRemoteNode()->getModule()->getFullName(), neighbor))
            return check_and_cast<Link *>(static_cast<Topology::Link *>(node->getLinkOut(i)));
    return nullptr;
}

NetworkConfiguratorBase::Link *NetworkConfiguratorBase::findLinkOut(const Node *node, const Node *neighbor) const
{
    for (int i = 0; i < node->getNumOutLinks(); i++)
        if (node->getLinkOut(i)->getLinkOutRemoteNode() == neighbor)
            return check_and_cast<Link *>(static_cast<Topology::Link *>(node->getLinkOut(i)));
    return nullptr;
}

NetworkConfiguratorBase::Link *NetworkConfiguratorBase::findLinkOut(const Interface *interface) const
{
    for (int i = 0; i < interface->node->getNumOutLinks(); i++) {
        auto link = check_and_cast<Link *>(static_cast<Topology::Link *>(interface->node->getLinkOut(i)));
        if (link->sourceInterface == interface)
            return link;
    }
    return nullptr;
}

Topology::Link *NetworkConfiguratorBase::findLinkOut(const Node *node, int gateId) const
{
    for (int i = 0; i < node->getNumOutLinks(); i++)
        if (node->getLinkOut(i)->getLinkOutLocalGateId() == gateId)
            return node->getLinkOut(i);
    return nullptr;
}

NetworkConfiguratorBase::Interface *NetworkConfiguratorBase::findInterface(const Node *node, NetworkInterface *networkInterface) const
{
    if (networkInterface == nullptr)
        return nullptr;
    for (auto& interface : node->interfaces)
        if (interface->networkInterface == networkInterface)
            return interface;
    return nullptr;
}

} // namespace inet

