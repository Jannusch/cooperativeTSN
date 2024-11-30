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

#include "cooperative_tsn/configurator/gatescheduling/TSNschedGrpcGateScheduleConfigurator.h"

#include "inet/linklayer/configurator/gatescheduling/base/GateScheduleConfiguratorBase.h"

#include <cstdio>
#include <fstream>

namespace inet {

Define_Module(TSNschedGrpcGateScheduleConfigurator);

static void printJson(std::ostream& stream, const cValue& value, int level = 0)
{
    std::string indent(level * 2, ' ');
    if (value.containsObject()) {
        auto object = value.objectValue();
        if (auto array = dynamic_cast<cValueArray*>(object)) {
            if (array->size() == 0)
                stream << "[]";
            else {
                stream << "[\n";
                for (int i = 0; i < array->size(); i++) {
                    if (i != 0)
                        stream << ",\n";
                    stream << indent << "  ";
                    printJson(stream, array->get(i), level + 1);
                }
                stream << "\n"
                    << indent << "]";
            }
        }
        else if (auto map = dynamic_cast<cValueMap*>(object)) {
            if (map->size() == 0)
                stream << "{}";
            else {
                stream << "{\n";
                auto it = map->getFields().begin();
                for (int i = 0; i < map->size(); i++, ++it) {
                    if (i != 0)
                        stream << ",\n";
                    stream << indent << " \"" << it->first << "\": ";
                    printJson(stream, it->second, level + 1);
                }
                stream << "\n"
                    << indent << "}";
            }
        }
        else
            throw cRuntimeError("Unknown object type");
    }
    else
        stream << value.str();
}

cValueMap* TSNschedGrpcGateScheduleConfigurator::convertInputToJson(const GateScheduleConfiguratorBase::Input& input) const
{

    cValueMap* json = new cValueMap();
    cValueArray* jsonDevices = new cValueArray();
    json->set("devices", jsonDevices);
    for (auto device : input.devices) {
        cValueMap* jsonDevice = new cValueMap();
        jsonDevices->add(jsonDevice);
        jsonDevice->set("name", device->module->getFullName());
    }
    cValueArray* jsonSwitches = new cValueArray();
    json->set("switches", jsonSwitches);
    for (int i = 0; i < input.switches.size(); i++) {
        auto switch_ = input.switches[i];
        cValueMap* jsonSwitch = new cValueMap();
        jsonSwitches->add(jsonSwitch);
        // TODO KLUDGE this is a wild guess
        double guardBand = 0;
        for (auto flow : input.flows) {
            double v = b(flow->startApplication->packetLength).get();
            if (guardBand < v)
                guardBand = v;
        }
        auto jsonPorts = new cValueArray();
        jsonSwitch->set("name", switch_->module->getFullName());
        jsonSwitch->set("ports", jsonPorts);
        for (int j = 0; j < switch_->ports.size(); j++) {
            auto port = switch_->ports[j];
            auto jsonPort = new cValueMap();
            jsonPorts->add(jsonPort);
            // KLUDGE: port name should not be unique in the network but only in the network node
            std::string nodeName = port->startNode->module->getFullName();
            jsonPort->set("name", nodeName + "-" + port->module->getFullName());
            jsonPort->set("connectsTo", port->endNode->module->getFullName());
            jsonPort->set("timeToTravel", port->propagationTime.dbl() * 1000000);
            jsonPort->set("timeToTravelUnit", "us");
            //            jsonPort->set("guardBandSize", guardBand);
            //            jsonPort->set("guardBandSizeUnit", "bit");
            jsonPort->set("portSpeed", bps(port->datarate).get() / 1000000);
            jsonPort->set("portSpeedSizeUnit", "bit");
            jsonPort->set("portSpeedTimeUnit", "us");
            jsonPort->set("scheduleType", "Hypercycle");
            jsonPort->set("cycleStart", 0);
            jsonPort->set("cycleStartUnit", "us");
            jsonPort->set("maximumSlotDuration", gateCycleDuration.dbl() * 1000000);
            jsonPort->set("maximumSlotDurationUnit", "us");
        }
    }
    cValueArray* jsonFlows = new cValueArray();
    json->set("flows", jsonFlows);
    for (auto flow : input.flows) {

        SimTime startTime = -1;
        int flowIndex = 0;
        for (int k = 0; k < configuration->size(); k++) {
            auto entry = check_and_cast<cValueMap*>(configuration->get(k).objectValue());
            auto name = entry->containsKey("name") ? entry->get("name").stringValue() : (std::string("flow") + std::to_string(flowIndex++)).c_str();
            if (name == flow->name)
                startTime = entry->containsKey("startTime") ? entry->get("startTime").doubleValue() : -1;
        }

        cValueMap* jsonFlow = new cValueMap();
        jsonFlows->add(jsonFlow);
        jsonFlow->set("name", flow->name);
        jsonFlow->set("type", "unicast");
        jsonFlow->set("sourceDevice", flow->startApplication->device->module->getFullName());
        jsonFlow->set("fixedPriority", "true");
        jsonFlow->set("priorityValue", flow->gateIndex);
        jsonFlow->set("packetPeriodicity", flow->startApplication->packetInterval.dbl() * 1000000);
        jsonFlow->set("packetPeriodicityUnit", "us");
        jsonFlow->set("packetSize", b(flow->startApplication->packetLength).get());
        jsonFlow->set("packetSizeUnit", "bit");
        // jsonFlow->set("hardConstraintTime", flow->startApplication->maxLatency.dbl() * 1000000);
        //  TODO: this needs to be fixed! This is only a hard coded value that will work for now

        if (startTime > 0) {
            jsonFlow->set("hardConstraintTime", 100);
            jsonFlow->set("hardConstraintTimeUnit", "us");
            jsonFlow->set("firstSendingTime", startTime.str());
            jsonFlow->set("firstSendingTimeUnit", "s");
        }
        else {
            jsonFlow->set("hardConstraintTime", 2000);
            jsonFlow->set("hardConstraintTimeUnit", "us");
        }
        cValueArray* endDevices = new cValueArray();
        jsonFlow->set("endDevices", endDevices);
        endDevices->add(cValue(flow->endDevice->module->getFullName()));
        cValueArray* hops = new cValueArray();
        jsonFlow->set("hops", hops);
        for (int j = 0; j < flow->pathFragments.size(); j++) {
            auto pathFragment = flow->pathFragments[j];
            for (int k = 0; k < pathFragment->networkNodes.size() - 1; k++) {
                auto networkNode = pathFragment->networkNodes[k];
                auto nextNetworkNode = pathFragment->networkNodes[k + 1];
                cValueMap* hop = new cValueMap();
                hops->add(hop);
                hop->set("currentNodeName", networkNode->module->getFullName());
                hop->set("nextNodeName", nextNetworkNode->module->getFullName());
            }
        }
    }
    return json;
}

void TSNschedGrpcGateScheduleConfigurator::writeInputToFile(const Input& input, std::string fileName) const
{
    auto json = convertInputToJson(input);
    std::ofstream stream;
    stream.open(fileName.c_str());
    if (stream.fail())
        throw cRuntimeError("Cannot open file %s", fileName.c_str());
    printJson(stream, cValue(json));
    delete json;
}

void TSNschedGrpcGateScheduleConfigurator::executeTSNsched(std::string inputFileName) const
{
    // TODO: for fututer something thats not so hacky
    // best case gRPC in C++
    std::string path = par("pathToCient").stringValue();
    std::string url = par("grpcAddress").stringValue() + std::string(":") + par("grpcPort").stringValue();
    std::string command = std::string(". ") + path + std::string("../.venv/bin/activate && python3 ") + path + std::string("client.py --url ") + url + std::string(" --input ") + inputFileName + std::string(" --output ") + inputFileName + std::string(".output >> output.txt 2>&1");
    if (std::system(command.c_str()) != 0)
        throw cRuntimeError("TSNsched command execution failed, make sure TSNsched is running and the url is correct.");
}

TSNschedGateScheduleConfigurator::Output* TSNschedGrpcGateScheduleConfigurator::computeGateScheduling(const Input& input) const
{
    auto parent_name = std::string(this->getParentModule()->getFullName());
    std::string baseName = getEnvir()->getConfig()->substituteVariables("${resultdir}/${configname}-${iterationvarsf}#${repetition}") + parent_name;
    std::string inputFileName = baseName + "-TSNsched-input.json";
    std::string outputFileName = inputFileName + ".output";
    std::remove(outputFileName.c_str());
    writeInputToFile(input, inputFileName);
    executeTSNsched(inputFileName);
    return readOutputFromFile(input, outputFileName);
}

} // namespace inet
