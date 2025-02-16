﻿/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io

* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "integrationpluginzigbeephilipshue.h"
#include "plugininfo.h"

#include <hardware/zigbee/zigbeehardwareresource.h>
#include <zcl/general/zigbeeclusteronoff.h>
#include <zcl/general/zigbeeclusterpowerconfiguration.h>
#include <zcl/general/zigbeeclusterlevelcontrol.h>
#include <zcl/measurement/zigbeeclusteroccupancysensing.h>
#include <zcl/measurement/zigbeeclustertemperaturemeasurement.h>
#include <zcl/measurement/zigbeeclusterilluminancemeasurement.h>
#include <zcl/manufacturerspecific/philips/zigbeeclustermanufacturerspecificphilips.h>


#include <math.h>

IntegrationPluginZigbeePhilipsHue::IntegrationPluginZigbeePhilipsHue()
{
    m_ieeeAddressParamTypeIds[dimmerSwitchThingClassId] = dimmerSwitchThingIeeeAddressParamTypeId;
    m_ieeeAddressParamTypeIds[dimmerSwitch2ThingClassId] = dimmerSwitch2ThingIeeeAddressParamTypeId;
    m_ieeeAddressParamTypeIds[smartButtonThingClassId] = smartButtonThingIeeeAddressParamTypeId;
    m_ieeeAddressParamTypeIds[motionSensorThingClassId] = motionSensorThingIeeeAddressParamTypeId;
    m_ieeeAddressParamTypeIds[wallSwitchModuleThingClassId] = wallSwitchModuleThingIeeeAddressParamTypeId;

    m_networkUuidParamTypeIds[dimmerSwitchThingClassId] = dimmerSwitchThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[dimmerSwitch2ThingClassId] = dimmerSwitch2ThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[smartButtonThingClassId] = smartButtonThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[motionSensorThingClassId] = motionSensorThingNetworkUuidParamTypeId;
    m_networkUuidParamTypeIds[wallSwitchModuleThingClassId] = wallSwitchModuleThingNetworkUuidParamTypeId;

    m_connectedStateTypeIds[dimmerSwitchThingClassId] = dimmerSwitchConnectedStateTypeId;
    m_connectedStateTypeIds[dimmerSwitch2ThingClassId] = dimmerSwitch2ConnectedStateTypeId;
    m_connectedStateTypeIds[smartButtonThingClassId] = smartButtonConnectedStateTypeId;
    m_connectedStateTypeIds[motionSensorThingClassId] = motionSensorConnectedStateTypeId;
    m_connectedStateTypeIds[wallSwitchModuleThingClassId] = wallSwitchModuleConnectedStateTypeId;

    m_signalStrengthStateTypeIds[dimmerSwitchThingClassId] = dimmerSwitchSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[dimmerSwitch2ThingClassId] = dimmerSwitch2SignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[smartButtonThingClassId] = smartButtonSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[motionSensorThingClassId] = motionSensorSignalStrengthStateTypeId;
    m_signalStrengthStateTypeIds[wallSwitchModuleThingClassId] = wallSwitchModuleSignalStrengthStateTypeId;

    m_versionStateTypeIds[dimmerSwitchThingClassId] = dimmerSwitchVersionStateTypeId;
    m_versionStateTypeIds[dimmerSwitch2ThingClassId] = dimmerSwitch2VersionStateTypeId;
    m_versionStateTypeIds[smartButtonThingClassId] = smartButtonVersionStateTypeId;
    m_versionStateTypeIds[motionSensorThingClassId] = motionSensorVersionStateTypeId;
    m_versionStateTypeIds[wallSwitchModuleThingClassId] = wallSwitchModuleVersionStateTypeId;
}

QString IntegrationPluginZigbeePhilipsHue::name() const
{
    return "Philips Hue";
}

bool IntegrationPluginZigbeePhilipsHue::handleNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    // Make sure this is from Philips 0x100b
    if (node->nodeDescriptor().manufacturerCode != Zigbee::Philips) {
        qCDebug(dcZigbeePhilipsHue()) << "Manufacturer code not matching. Ignoring node." << node->nodeDescriptor().manufacturerCode;
        return false;
    }

    if (node->endpoints().count() == 2 && node->hasEndpoint(0x01) && node->hasEndpoint(0x02)) {
        ZigbeeNodeEndpoint *endpointOne = node->getEndpoint(0x01);
        ZigbeeNodeEndpoint *endpointTwo = node->getEndpoint(0x02);

        // Dimmer switch
        if (endpointOne->profile() == Zigbee::ZigbeeProfileLightLink &&
                endpointOne->deviceId() == Zigbee::LightLinkDeviceNonColourSceneController &&
                endpointTwo->profile() == Zigbee::ZigbeeProfileHomeAutomation &&
                endpointTwo->deviceId() == Zigbee::HomeAutomationDeviceSimpleSensor) {

            qCDebug(dcZigbeePhilipsHue()) << "Handling Hue dimmer switch" << node << endpointOne << endpointTwo;
            createThing(dimmerSwitchThingClassId, networkUuid, node);
            initDimmerSwitch(node, endpointTwo);
            return true;
        }

        // Outdoor sensor
        if (endpointOne->profile() == Zigbee::ZigbeeProfileLightLink &&
                endpointOne->deviceId() == Zigbee::LightLinkDeviceOnOffSensor &&
                endpointTwo->profile() == Zigbee::ZigbeeProfileHomeAutomation &&
                endpointTwo->deviceId() == Zigbee::HomeAutomationDeviceOccupacySensor) {

            qCDebug(dcZigbeePhilipsHue()) << "Handling Hue outdoor sensor" << node << endpointOne << endpointTwo;
            createThing(motionSensorThingClassId, networkUuid, node);
            initMotionSensor(node);
            return true;
        }
    }

    ZigbeeNodeEndpoint *endpointOne = node->getEndpoint(0x01);
    if (endpointOne) {

        if (endpointOne->modelIdentifier() == "RWL022") {
            createThing(dimmerSwitch2ThingClassId, networkUuid, node);
            initDimmerSwitch(node, endpointOne);
            return true;
        }

        // Smart buttton
        if (endpointOne->profile() == Zigbee::ZigbeeProfileHomeAutomation &&
                endpointOne->deviceId() == Zigbee::HomeAutomationDeviceNonColourSceneController) {
            qCDebug(dcZigbeePhilipsHue()) << "Handling Hue Smart button" << node << endpointOne;
            createThing(smartButtonThingClassId, networkUuid, node);
            initSmartButton(node);
            return true;
        }

        // Wall switch module
        if (endpointOne->profile() == Zigbee::ZigbeeProfileHomeAutomation &&
                endpointOne->deviceId() == Zigbee::HomeAutomationDeviceNonColourController) {
            createThing(wallSwitchModuleThingClassId, networkUuid, node);
            initWallSwitchModule(node);
            return true;
        }
    }

    qCWarning(dcZigbeePhilipsHue()) << "Device manufacturer code matches Philips/Signify, but node not handled by plugin:" << node->modelName();

    return false;
}

void IntegrationPluginZigbeePhilipsHue::handleRemoveNode(ZigbeeNode *node, const QUuid &networkUuid)
{
    Q_UNUSED(networkUuid)

    if (m_thingNodes.values().contains(node)) {
        Thing *thing = m_thingNodes.key(node);
        qCDebug(dcZigbeePhilipsHue()) << node << "for" << thing << "has left the network.";
        m_thingNodes.remove(thing);
        emit autoThingDisappeared(thing->id());
    }
}

void IntegrationPluginZigbeePhilipsHue::init()
{
    hardwareManager()->zigbeeResource()->registerHandler(this, ZigbeeHardwareResource::HandlerTypeVendor);
}

void IntegrationPluginZigbeePhilipsHue::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    QUuid networkUuid = thing->paramValue(m_networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
    ZigbeeAddress zigbeeAddress = ZigbeeAddress(thing->paramValue(m_ieeeAddressParamTypeIds.value(thing->thingClassId())).toString());
    ZigbeeNode *node = hardwareManager()->zigbeeResource()->claimNode(this, networkUuid, zigbeeAddress);
    if (!node) {
        qCWarning(dcZigbeePhilipsHue()) << "Zigbee node for" << info->thing()->name() << "not found.";
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }
    m_thingNodes.insert(thing, node);

    // Update connected state
    thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), node->reachable());
    connect(node, &ZigbeeNode::reachableChanged, thing, [thing, this](bool reachable){
        thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), reachable);
    });

    // Update signal strength
    thing->setStateValue(m_signalStrengthStateTypeIds.value(thing->thingClassId()), qRound(node->lqi() * 100.0 / 255.0));
    connect(node, &ZigbeeNode::lqiChanged, thing, [this, thing](quint8 lqi){
        uint signalStrength = qRound(lqi * 100.0 / 255.0);
        qCDebug(dcZigbeePhilipsHue()) << thing << "signal strength changed" << signalStrength << "%";
        thing->setStateValue(m_signalStrengthStateTypeIds.value(thing->thingClassId()), signalStrength);
    });

    // Thing specific setup
    if (thing->thingClassId() == dimmerSwitchThingClassId) {
        ZigbeeNodeEndpoint *endpointZll = node->getEndpoint(0x01);
        ZigbeeNodeEndpoint *endpointHa = node->getEndpoint(0x02);

        // Set the version
        thing->setStateValue(m_versionStateTypeIds.value(thing->thingClassId()), endpointZll->softwareBuildId());

        ZigbeeClusterManufacturerSpecificPhilips *philipsCluster = endpointHa->inputCluster<ZigbeeClusterManufacturerSpecificPhilips>(ZigbeeClusterLibrary::ClusterIdManufacturerSpecificPhilips);
        if (!philipsCluster) {
            qCWarning(dcZigbeePhilipsHue()) << "Could not find Manufacturer Specific (Philips) cluster on" << thing << endpointHa;
        } else {
            connect(philipsCluster, &ZigbeeClusterManufacturerSpecificPhilips::buttonPressed, thing, [=](quint8 button, ZigbeeClusterManufacturerSpecificPhilips::Operation operation) {
                qCDebug(dcZigbeePhilipsHue()) << "Button" << button << operation;
                QHash<quint8, QString> buttonMap = {
                    {1, "ON"},
                    {2, "DIM UP"},
                    {3, "DIM DOWN"},
                    {4, "OFF"}
                };
                switch (operation) {
                case ZigbeeClusterManufacturerSpecificPhilips::OperationButtonPress:
                    // This doesn't appear on very quick press/release. But we always get the short release, so let's use that instead
                    break;
                case ZigbeeClusterManufacturerSpecificPhilips::OperationButtonShortRelease:
                    thing->emitEvent(dimmerSwitchPressedEventTypeId, ParamList() << Param(dimmerSwitchPressedEventButtonNameParamTypeId, buttonMap.value(button)));
                    break;
                case ZigbeeClusterManufacturerSpecificPhilips::OperationButtonHold:
                    thing->emitEvent(dimmerSwitchLongPressedEventTypeId, ParamList() << Param(dimmerSwitchLongPressedEventButtonNameParamTypeId, buttonMap.value(button)));
                    break;
                case ZigbeeClusterManufacturerSpecificPhilips::OperationButtonLongRelease:
                    // Release after a longpress. But for longpresses we always seem to get the Hold before, so we'll use that.
                    break;
                }
            });
        }


        // Get battery level changes
        ZigbeeClusterPowerConfiguration *powerCluster = endpointHa->inputCluster<ZigbeeClusterPowerConfiguration>(ZigbeeClusterLibrary::ClusterIdPowerConfiguration);
        if (!powerCluster) {
            qCWarning(dcZigbeePhilipsHue()) << "Could not find power configuration cluster on" << thing << endpointHa;
        } else {
            // Only set the initial state if the attribute already exists
            if (powerCluster->hasAttribute(ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining)) {
                thing->setStateValue(dimmerSwitchBatteryLevelStateTypeId, powerCluster->batteryPercentage());
                thing->setStateValue(dimmerSwitchBatteryCriticalStateTypeId, (powerCluster->batteryPercentage() < 10.0));
            }

            connect(powerCluster, &ZigbeeClusterPowerConfiguration::batteryPercentageChanged, thing, [=](double percentage){
                qCDebug(dcZigbeePhilipsHue()) << "Battery percentage changed" << percentage << "%" << thing;
                thing->setStateValue(dimmerSwitchBatteryLevelStateTypeId, percentage);
                thing->setStateValue(dimmerSwitchBatteryCriticalStateTypeId, (percentage < 10.0));
            });
        }
    }

    // Thing specific setup
    if (thing->thingClassId() == dimmerSwitch2ThingClassId) {
        ZigbeeNodeEndpoint *endpoint = node->getEndpoint(0x01);

        // Set the version
        thing->setStateValue(m_versionStateTypeIds.value(thing->thingClassId()), endpoint->softwareBuildId());

        ZigbeeClusterManufacturerSpecificPhilips *philipsCluster = endpoint->inputCluster<ZigbeeClusterManufacturerSpecificPhilips>(ZigbeeClusterLibrary::ClusterIdManufacturerSpecificPhilips);
        if (!philipsCluster) {
            qCWarning(dcZigbeePhilipsHue()) << "Could not find Manufacturer Specific (Philips) cluster on" << thing << endpoint;
        } else {
            connect(philipsCluster, &ZigbeeClusterManufacturerSpecificPhilips::buttonPressed, thing, [=](quint8 button, ZigbeeClusterManufacturerSpecificPhilips::Operation operation) {
                qCDebug(dcZigbeePhilipsHue()) << "Button" << button << operation;
                QHash<quint8, QString> buttonMap = {
                    {1, "POWER"},
                    {2, "DIM UP"},
                    {3, "DIM DOWN"},
                    {4, "HUE"}
                };
                switch (operation) {
                case ZigbeeClusterManufacturerSpecificPhilips::OperationButtonPress:
                    // This doesn't appear on very quick press/release. But we always get the short release, so let's use that instead
                    break;
                case ZigbeeClusterManufacturerSpecificPhilips::OperationButtonShortRelease:
                    thing->emitEvent(dimmerSwitch2PressedEventTypeId, ParamList() << Param(dimmerSwitch2PressedEventButtonNameParamTypeId, buttonMap.value(button)));
                    break;
                case ZigbeeClusterManufacturerSpecificPhilips::OperationButtonHold:
                    thing->emitEvent(dimmerSwitch2LongPressedEventTypeId, ParamList() << Param(dimmerSwitch2LongPressedEventButtonNameParamTypeId, buttonMap.value(button)));
                    break;
                case ZigbeeClusterManufacturerSpecificPhilips::OperationButtonLongRelease:
                    // Release after a longpress. But for longpresses we always seem to get the Hold before, so we'll use that.
                    break;
                }
            });
        }


        // Get battery level changes
        ZigbeeClusterPowerConfiguration *powerCluster = endpoint->inputCluster<ZigbeeClusterPowerConfiguration>(ZigbeeClusterLibrary::ClusterIdPowerConfiguration);
        if (!powerCluster) {
            qCWarning(dcZigbeePhilipsHue()) << "Could not find power configuration cluster on" << thing << endpoint;
        } else {
            // Only set the initial state if the attribute already exists
            if (powerCluster->hasAttribute(ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining)) {
                thing->setStateValue(dimmerSwitch2BatteryLevelStateTypeId, powerCluster->batteryPercentage());
                thing->setStateValue(dimmerSwitch2BatteryCriticalStateTypeId, (powerCluster->batteryPercentage() < 10.0));
            }

            connect(powerCluster, &ZigbeeClusterPowerConfiguration::batteryPercentageChanged, thing, [=](double percentage){
                qCDebug(dcZigbeePhilipsHue()) << "Battery percentage changed" << percentage << "%" << thing;
                thing->setStateValue(dimmerSwitch2BatteryLevelStateTypeId, percentage);
                thing->setStateValue(dimmerSwitch2BatteryCriticalStateTypeId, (percentage < 10.0));
            });
        }
    }


    if (thing->thingClassId() == smartButtonThingClassId) {
        ZigbeeNodeEndpoint *endpointHa = node->getEndpoint(0x01);

        // Set the version
        thing->setStateValue(m_versionStateTypeIds.value(thing->thingClassId()), endpointHa->softwareBuildId());

        // Connect to button presses
        ZigbeeClusterOnOff *onOffCluster = endpointHa->outputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
        if (!onOffCluster) {
            qCWarning(dcZigbeePhilipsHue()) << "Could not find on/off client cluster on" << thing << endpointHa;
        } else {
            // The smart button toggles between command(On) and commandOffWithEffect() for short presses...
            connect(onOffCluster, &ZigbeeClusterOnOff::commandSent, thing, [=](ZigbeeClusterOnOff::Command command){
                if (command == ZigbeeClusterOnOff::CommandOn) {
                    qCDebug(dcZigbeePhilipsHue()) << thing << "pressed";
                    emit emitEvent(Event(smartButtonPressedEventTypeId, thing->id()));
                } else {
                    qCWarning(dcZigbeePhilipsHue()) << thing << "unhandled command received" << command;
                }
            });
            connect(onOffCluster, &ZigbeeClusterOnOff::commandOffWithEffectSent, thing, [=](ZigbeeClusterOnOff::Effect effect, quint8 effectVariant){
                qCDebug(dcZigbeePhilipsHue()) << thing << "pressed" << effect << effectVariant;
                emit emitEvent(Event(smartButtonPressedEventTypeId, thing->id()));
            });

            // ...and toggless between level up/down for long presses
            ZigbeeClusterLevelControl *levelCluster = endpointHa->outputCluster<ZigbeeClusterLevelControl>(ZigbeeClusterLibrary::ClusterIdLevelControl);
            if (!levelCluster) {
                qCWarning(dcZigbeePhilipsHue()) << "Could not find level client cluster on" << thing << endpointHa;
            } else {
                connect(levelCluster, &ZigbeeClusterLevelControl::commandStepSent, thing, [=](bool withOnOff, ZigbeeClusterLevelControl::StepMode stepMode, quint8 stepSize, quint16 transitionTime){
                    qCDebug(dcZigbeePhilipsHue()) << thing << "level button pressed" << withOnOff << stepMode << stepSize << transitionTime;
                    switch (stepMode) {
                    case ZigbeeClusterLevelControl::StepModeUp:
                        qCDebug(dcZigbeePhilipsHue()) << thing << "DIM UP pressed";
                        emit emitEvent(Event(smartButtonLongPressedEventTypeId, thing->id()));
                        break;
                    case ZigbeeClusterLevelControl::StepModeDown:
                        qCDebug(dcZigbeePhilipsHue()) << thing << "DIM DOWN pressed";
                        emit emitEvent(Event(smartButtonLongPressedEventTypeId, thing->id()));
                        break;
                    }
                });
            }
        }

        // Get battery level changes
        ZigbeeClusterPowerConfiguration *powerCluster = endpointHa->inputCluster<ZigbeeClusterPowerConfiguration>(ZigbeeClusterLibrary::ClusterIdPowerConfiguration);
        if (!powerCluster) {
            qCWarning(dcZigbeePhilipsHue()) << "Could not find power configuration cluster on" << thing << endpointHa;
        } else {
            // Only set the initial state if the attribute already exists
            if (powerCluster->hasAttribute(ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining)) {
                thing->setStateValue(smartButtonBatteryLevelStateTypeId, powerCluster->batteryPercentage());
                thing->setStateValue(smartButtonBatteryCriticalStateTypeId, (powerCluster->batteryPercentage() < 10.0));
            }

            connect(powerCluster, &ZigbeeClusterPowerConfiguration::batteryPercentageChanged, thing, [=](double percentage){
                qCDebug(dcZigbeePhilipsHue()) << "Battery percentage changed" << percentage << "%" << thing;
                thing->setStateValue(smartButtonBatteryLevelStateTypeId, percentage);
                thing->setStateValue(smartButtonBatteryCriticalStateTypeId, (percentage < 10.0));
            });
        }
    }

    if (thing->thingClassId() == wallSwitchModuleThingClassId) {
        ZigbeeNodeEndpoint *endpointHa = node->getEndpoint(0x01);

        // Set the version
        thing->setStateValue(m_versionStateTypeIds.value(thing->thingClassId()), endpointHa->softwareBuildId());

        // Connect to the manufactuer specific cluster
        ZigbeeClusterManufacturerSpecificPhilips *philipsCluster = endpointHa->inputCluster<ZigbeeClusterManufacturerSpecificPhilips>(ZigbeeClusterLibrary::ClusterIdManufacturerSpecificPhilips);
        if (!philipsCluster) {
            qCWarning(dcZigbeePhilipsHue()) << "Could not find Manufacturer Specific (Philips) cluster on" << thing << endpointHa;
        } else {
            qCDebug(dcZigbeePhilipsHue()) << "Connecting to manufacturer specific cluster";
            connect(philipsCluster, &ZigbeeClusterManufacturerSpecificPhilips::buttonPressed, thing, [=](quint8 button, ZigbeeClusterManufacturerSpecificPhilips::Operation operation) {
                qCDebug(dcZigbeePhilipsHue()) << "Button" << button << operation;
                switch (operation) {
                case ZigbeeClusterManufacturerSpecificPhilips::OperationButtonPress:
                    // Unused (could be used for a bool "pressed" state type)
                    break;
                case ZigbeeClusterManufacturerSpecificPhilips::OperationButtonShortRelease:
                    thing->emitEvent(wallSwitchModulePressedEventTypeId, ParamList() << Param(wallSwitchModulePressedEventButtonNameParamTypeId, QString::number(button)));
                    break;
                case ZigbeeClusterManufacturerSpecificPhilips::OperationButtonHold:
                    thing->emitEvent(wallSwitchModuleLongPressedEventTypeId, ParamList() << Param(wallSwitchModuleLongPressedEventButtonNameParamTypeId, QString::number(button)));
                    break;
                case ZigbeeClusterManufacturerSpecificPhilips::OperationButtonLongRelease:
                    // Release after a longpress. But for longpresses we always seem to get the Hold before, so we'll use that.
                    break;
                }
            });
        }

        // Get battery level changes
        ZigbeeClusterPowerConfiguration *powerCluster = endpointHa->inputCluster<ZigbeeClusterPowerConfiguration>(ZigbeeClusterLibrary::ClusterIdPowerConfiguration);
        if (!powerCluster) {
            qCWarning(dcZigbeePhilipsHue()) << "Could not find power configuration cluster on" << thing << endpointHa;
        } else {
            // Only set the initial state if the attribute already exists
            if (powerCluster->hasAttribute(ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining)) {
                thing->setStateValue(wallSwitchModuleBatteryLevelStateTypeId, powerCluster->batteryPercentage());
                thing->setStateValue(wallSwitchModuleBatteryCriticalStateTypeId, (powerCluster->batteryPercentage() < 10.0));
            }

            connect(powerCluster, &ZigbeeClusterPowerConfiguration::batteryPercentageChanged, thing, [=](double percentage){
                qCDebug(dcZigbeePhilipsHue()) << "Battery percentage changed" << percentage << "%" << thing;
                thing->setStateValue(wallSwitchModuleBatteryLevelStateTypeId, percentage);
                thing->setStateValue(wallSwitchModuleBatteryCriticalStateTypeId, (percentage < 10.0));
            });
        }

        // Button mode settings
        connect(thing, &Thing::settingChanged, this, [=](const ParamTypeId &settingTypeId, const QVariant &value){
            if (settingTypeId == wallSwitchModuleSettingsButtonModeParamTypeId) {
                QHash<quint8, QString> modeMap = {
                    {0x00, "Single rocker"},
                    {0x01, "Single push button"},
                    {0x02, "Dual rocker"},
                    {0x03, "Dual push button"}
                };
                quint8 buttonMode = modeMap.key(value.toString());
                QList<ZigbeeClusterLibrary::WriteAttributeRecord> attributes;
                ZigbeeClusterLibrary::WriteAttributeRecord deviceModeAttribute;
                deviceModeAttribute.attributeId = 0x0034;
                deviceModeAttribute.dataType = Zigbee::Enum8;
                deviceModeAttribute.data = ZigbeeDataType(buttonMode).data();
                attributes.append(deviceModeAttribute);

                qCDebug(dcZigbeePhilipsHue()) << "Setting device mode config to" << value.toString();
                ZigbeeClusterBasic *basicCluster = endpointHa->inputCluster<ZigbeeClusterBasic>(ZigbeeClusterLibrary::ClusterIdBasic);
                ZigbeeClusterReply *deviceModeConfigReply = basicCluster->writeAttributes(attributes, Zigbee::Philips);
                connect(deviceModeConfigReply, &ZigbeeClusterReply::finished, node, [deviceModeConfigReply](){
                    qCDebug(dcZigbeePhilipsHue()) << "Writing device mode configuration finished:" << deviceModeConfigReply->error();
                });
            }
        });
    }

    if (thing->thingClassId() == motionSensorThingClassId) {
        ZigbeeNodeEndpoint *endpointHa = node->getEndpoint(0x02);

        // Set the version
        thing->setStateValue(m_versionStateTypeIds.value(thing->thingClassId()), endpointHa->softwareBuildId());

        // Get battery level changes
        ZigbeeClusterPowerConfiguration *powerCluster = endpointHa->outputCluster<ZigbeeClusterPowerConfiguration>(ZigbeeClusterLibrary::ClusterIdPowerConfiguration);
        if (!powerCluster) {
            qCWarning(dcZigbeePhilipsHue()) << "Could not find power configuration cluster on" << thing << endpointHa;
        } else {
            // Only set the initial state if the attribute already exists
            if (powerCluster->hasAttribute(ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining)) {
                thing->setStateValue(motionSensorBatteryLevelStateTypeId, powerCluster->batteryPercentage());
                thing->setStateValue(motionSensorBatteryCriticalStateTypeId, (powerCluster->batteryPercentage() < 10.0));
            }

            connect(powerCluster, &ZigbeeClusterPowerConfiguration::batteryPercentageChanged, thing, [=](double percentage){
                qCDebug(dcZigbeePhilipsHue()) << "Battery percentage changed" << percentage << "%" << thing;
                thing->setStateValue(motionSensorBatteryLevelStateTypeId, percentage);
                thing->setStateValue(motionSensorBatteryCriticalStateTypeId, (percentage < 10.0));
            });
        }

        ZigbeeClusterOccupancySensing *occupancyCluster = endpointHa->inputCluster<ZigbeeClusterOccupancySensing>(ZigbeeClusterLibrary::ClusterIdOccupancySensing);
        if (!occupancyCluster) {
            qCWarning(dcZigbeePhilipsHue()) << "Occupancy cluster not found on" << thing;
        } else {
            if (!m_presenceTimer) {
                m_presenceTimer = hardwareManager()->pluginTimerManager()->registerTimer(1);
            }

            connect(m_presenceTimer, &PluginTimer::timeout, thing, [thing](){
                if (thing->stateValue(motionSensorIsPresentStateTypeId).toBool()) {
                    int timeout = thing->setting(motionSensorSettingsTimeoutParamTypeId).toInt();
                    QDateTime lastSeenTime = QDateTime::fromMSecsSinceEpoch(thing->stateValue(motionSensorLastSeenTimeStateTypeId).toULongLong() * 1000);
                    if (lastSeenTime.addSecs(timeout) < QDateTime::currentDateTime()) {
                        qCDebug(dcZigbeePhilipsHue()) << thing << "Presence timeout";
                        thing->setStateValue(motionSensorIsPresentStateTypeId, false);
                    }
                }
            });

            connect(occupancyCluster, &ZigbeeClusterOccupancySensing::occupancyChanged, thing, [=](bool occupancy){
                qCDebug(dcZigbeePhilipsHue()) << thing << "occupancy cluster changed" << occupancy;
                // Only change the state if the it changed to true, it will be disabled by the timer
                if (occupancy) {
                    thing->setStateValue(motionSensorIsPresentStateTypeId, true);
                    thing->setStateValue(motionSensorLastSeenTimeStateTypeId, QDateTime::currentMSecsSinceEpoch() / 1000);
                    m_presenceTimer->start();
                }
            });
        }

        ZigbeeClusterTemperatureMeasurement *temperatureCluster = endpointHa->inputCluster<ZigbeeClusterTemperatureMeasurement>(ZigbeeClusterLibrary::ClusterIdTemperatureMeasurement);
        if (!temperatureCluster) {
            qCWarning(dcZigbeePhilipsHue()) << "Could not find the temperature measurement server cluster on" << thing << endpointHa;
        } else {
            // Only set the state if the cluster actually has the attribute
            if (temperatureCluster->hasAttribute(ZigbeeClusterTemperatureMeasurement::AttributeMeasuredValue)) {
                thing->setStateValue(motionSensorTemperatureStateTypeId, temperatureCluster->temperature());
            }

            connect(temperatureCluster, &ZigbeeClusterTemperatureMeasurement::temperatureChanged, thing, [thing](double temperature){
                qCDebug(dcZigbeePhilipsHue()) << thing << "temperature changed" << temperature << "°C";
                thing->setStateValue(motionSensorTemperatureStateTypeId, temperature);
            });
        }

        ZigbeeClusterIlluminanceMeasurement *illuminanceCluster = endpointHa->inputCluster<ZigbeeClusterIlluminanceMeasurement>(ZigbeeClusterLibrary::ClusterIdIlluminanceMeasurement);
        if (!illuminanceCluster) {
            qCWarning(dcZigbeePhilipsHue()) << "Could not find the illuminance measurement server cluster on" << thing << endpointHa;
        } else {
            // Only set the state if the cluster actually has the attribute
            if (illuminanceCluster->hasAttribute(ZigbeeClusterIlluminanceMeasurement::AttributeMeasuredValue)) {
                int convertedValue = pow(10, illuminanceCluster->illuminance() / 10000) - 1;
                qCDebug(dcZigbeePhilipsHue()) << thing << "illuminance" << illuminanceCluster->illuminance() << "-->" << convertedValue << "lux";
                thing->setStateValue(motionSensorLightIntensityStateTypeId, convertedValue);
            }

            connect(illuminanceCluster, &ZigbeeClusterIlluminanceMeasurement::illuminanceChanged, thing, [thing](quint16 illuminance){
                int convertedValue = pow(10, illuminance / 10000) - 1;
                qCDebug(dcZigbeePhilipsHue()) << thing << "illuminance changed" << illuminance << "-->" << convertedValue << "lux";
                thing->setStateValue(motionSensorLightIntensityStateTypeId, convertedValue);
            });
        }


    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginZigbeePhilipsHue::executeAction(ThingActionInfo *info)
{
    info->finish(Thing::ThingErrorUnsupportedFeature);
}

void IntegrationPluginZigbeePhilipsHue::thingRemoved(Thing *thing)
{
    ZigbeeNode *node = m_thingNodes.take(thing);
    if (node) {
        QUuid networkUuid = thing->paramValue(m_networkUuidParamTypeIds.value(thing->thingClassId())).toUuid();
        hardwareManager()->zigbeeResource()->removeNodeFromNetwork(networkUuid, node);
    }
}

void IntegrationPluginZigbeePhilipsHue::createThing(const ThingClassId &thingClassId, const QUuid &networkUuid, ZigbeeNode *node)
{
    ThingDescriptor descriptor(thingClassId);
    QString deviceClassName = supportedThings().findById(thingClassId).displayName();
    descriptor.setTitle(deviceClassName);

    ParamList params;
    params.append(Param(m_networkUuidParamTypeIds[thingClassId], networkUuid.toString()));
    params.append(Param(m_ieeeAddressParamTypeIds[thingClassId], node->extendedAddress().toString()));
    descriptor.setParams(params);
    emit autoThingsAppeared({descriptor});
}

void IntegrationPluginZigbeePhilipsHue::initDimmerSwitch(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    qCDebug(dcZigbeePhilipsHue()) << "Clearing binding table on node" << node;
    ZigbeeReply *removeBindingsReply = node->removeAllBindings();
    connect(removeBindingsReply, &ZigbeeReply::finished, node, [=](){
        if (removeBindingsReply->error() != ZigbeeReply::ErrorNoError) {
            qCWarning(dcZigbeePhilipsHue()) << "Failed to remove all bindings for initialization of" << node;
        } else {
            qCDebug(dcZigbeePhilipsHue()) << "Removed all bindings successfully from" << node;
        }

        bindBatteryCluster(node, endpoint);

        qCDebug(dcZigbeePhilipsHue()) << "Binding Manufacturer specific cluster to coordinator";
        ZigbeeDeviceObjectReply *zdoReply = node->deviceObject()->requestBindGroupAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdManufacturerSpecificPhilips, 0x0000);
        connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
            if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                qCWarning(dcZigbeePhilipsHue()) << "Failed to bind manufacturer specific cluster to coordinator" << zdoReply->error();
            } else {
                qCDebug(dcZigbeePhilipsHue()) << "Binding manufacturer specific cluster to coordinator finished successfully";
            }

            qCDebug(dcZigbeePhilipsHue()) << "Reading binding table from node" << node;
            ZigbeeReply *reply = node->readBindingTableEntries();
            connect(reply, &ZigbeeReply::finished, node, [=](){
                if (reply->error() != ZigbeeReply::ErrorNoError) {
                    qCWarning(dcZigbeePhilipsHue()) << "Failed to read binding table from" << node;
                } else {
                    foreach (const ZigbeeDeviceProfile::BindingTableListRecord &binding, node->bindingTableRecords()) {
                        qCDebug(dcZigbeePhilipsHue()) << node << binding;
                    }
                }
            });
        });
    });
}

void IntegrationPluginZigbeePhilipsHue::initSmartButton(ZigbeeNode *node)
{
    ZigbeeNodeEndpoint *endpoint = node->getEndpoint(0x01);

    qCDebug(dcZigbeePhilipsHue()) << "Clearing binding table on node" << node;
    ZigbeeReply *removeBindingsReply = node->removeAllBindings();
    connect(removeBindingsReply, &ZigbeeReply::finished, node, [=](){
        if (removeBindingsReply->error() != ZigbeeReply::ErrorNoError) {
            qCWarning(dcZigbeePhilipsHue()) << "Failed to remove all bindings for initialization of" << node;
        } else {
            qCDebug(dcZigbeePhilipsHue()) << "Removed all bindings successfully from" << node;
        }

        bindBatteryCluster(node, endpoint);
        bindOnOffCluster(node, endpoint);
        bindLevelControlCluster(node, endpoint);
        dumpBindingTable(node);
    });
}

void IntegrationPluginZigbeePhilipsHue::initWallSwitchModule(ZigbeeNode *node)
{
    ZigbeeNodeEndpoint *endpoint = node->getEndpoint(0x01);

    qCDebug(dcZigbeePhilipsHue()) << "Clearing binding table on node" << node;
    ZigbeeReply *removeBindingsReply = node->removeAllBindings();
    connect(removeBindingsReply, &ZigbeeReply::finished, node, [=](){
        if (removeBindingsReply->error() != ZigbeeReply::ErrorNoError) {
            qCWarning(dcZigbeePhilipsHue()) << "Failed to remove all bindings for initialization of" << node;
        } else {
            qCDebug(dcZigbeePhilipsHue()) << "Removed all bindings successfully from" << node;
        }

        qCDebug(dcZigbeePhilipsHue()) << "Binding Manufacturer specific cluster from node" << node;
        ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdManufacturerSpecificPhilips, hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
        connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
            if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                qCWarning(dcZigbeePhilipsHue()) << "Failed to bind manufacturer specific cluster:" << zdoReply->error();
            } else {
                qCDebug(dcZigbeePhilipsHue()) << "Successfully bound manufacturer specific cluster.";
            }
        });


        qCDebug(dcZigbeePhilipsHue()) << "Configuring device for single rocker mode (default)";
        //        singlerocker 	0x00
        //        singlepushbutton 	0x01
        //        dualrocker 	0x02
        //        dualpushbutton 	0x03

        QList<ZigbeeClusterLibrary::WriteAttributeRecord> attributes;
        ZigbeeClusterLibrary::WriteAttributeRecord deviceModeAttribute;
        deviceModeAttribute.attributeId = 0x0034;
        deviceModeAttribute.dataType = Zigbee::Enum8;
        deviceModeAttribute.data = ZigbeeDataType(static_cast<quint8>(0x00)).data();
        attributes.append(deviceModeAttribute);

        ZigbeeClusterReply *deviceModeConfigReply = endpoint->inputCluster<ZigbeeClusterBasic>(ZigbeeClusterLibrary::ClusterIdBasic)->writeAttributes(attributes, Zigbee::Philips);
        connect(deviceModeConfigReply, &ZigbeeClusterReply::finished, node, [deviceModeConfigReply](){
            qCDebug(dcZigbeePhilipsHue()) << "Writing device mode configuration finished:" << deviceModeConfigReply->error();
        });


        bindBatteryCluster(node, endpoint);
        bindOnOffCluster(node, endpoint);
        bindLevelControlCluster(node, endpoint);

        dumpBindingTable(node);
    });
}

void IntegrationPluginZigbeePhilipsHue::bindBatteryCluster(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    // Read battery, bind and configure attribute reporting for battery
    if (!endpoint->hasInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)) {
        qCWarning(dcZigbeePhilipsHue()) << "Failed to initialize the power configuration cluster because the cluster could not be found" << node << endpoint;
        return;
    }

    // Bind the cluster to the coordinator
    qCDebug(dcZigbeePhilipsHue()) << "Binding power configuration cluster from node" << node;
    ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdPowerConfiguration,
                                                                                      hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
    connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
        if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
            qCWarning(dcZigbeePhilipsHue()) << "Failed to bind power cluster" << zdoReply->error();
        } else {
            qCDebug(dcZigbeePhilipsHue()) << "Successfully bound power configuration cluster.";
        }

        // Configure attribute reporting for battery remaining (0.5 % changes = 1)
        ZigbeeClusterLibrary::AttributeReportingConfiguration reportingConfig;
        reportingConfig.attributeId = ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining;
        reportingConfig.dataType = Zigbee::Uint8;
        reportingConfig.minReportingInterval = 300;
        reportingConfig.maxReportingInterval = 2700;
        reportingConfig.reportableChange = ZigbeeDataType(static_cast<quint8>(1)).data();

        qCDebug(dcZigbeePhilipsHue()) << "Configuring attribute reporting for power configuration cluster on node:" << node;
        ZigbeeClusterReply *reportingReply = endpoint->getInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)->configureReporting({reportingConfig});
        connect(reportingReply, &ZigbeeClusterReply::finished, this, [=](){
            if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigbeePhilipsHue()) << "Failed to configure power cluster attribute reporting" << reportingReply->error();
            } else {
                qCDebug(dcZigbeePhilipsHue()) << "Attribute reporting configuration finished for power cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
            }
        });
    });
}

void IntegrationPluginZigbeePhilipsHue::bindOnOffCluster(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    qCDebug(dcZigbeePhilipsHue()) << "Binding on/off cluster from node" << node;
    ZigbeeDeviceObjectReply * bindOnOffReply = node->deviceObject()->requestBindGroupAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdOnOff, 0x0000);
    connect(bindOnOffReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
        if (bindOnOffReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
            qCWarning(dcZigbeePhilipsHue()) << "Failed to bind on/off cluster:" << bindOnOffReply->error();
        } else {
            qCDebug(dcZigbeePhilipsHue()) << "Successfully bound on/off cluster.";
        }
    });
}

void IntegrationPluginZigbeePhilipsHue::bindLevelControlCluster(ZigbeeNode *node, ZigbeeNodeEndpoint *endpoint)
{
    qCDebug(dcZigbeePhilipsHue()) << "Binding level control cluster from node" << node;
    ZigbeeDeviceObjectReply * bindLevelControlReply = node->deviceObject()->requestBindGroupAddress(endpoint->endpointId(), ZigbeeClusterLibrary::ClusterIdLevelControl, 0x0000);
    connect(bindLevelControlReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
        if (bindLevelControlReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
            qCWarning(dcZigbeePhilipsHue()) << "Failed to bind level control cluster:" << bindLevelControlReply->error();
        } else {
            qCDebug(dcZigbeePhilipsHue()) << "Successfully bound level control cluster.";
        }
    });
}

void IntegrationPluginZigbeePhilipsHue::dumpBindingTable(ZigbeeNode *node)
{
    qCDebug(dcZigbeePhilipsHue()) << "Reading binding table from node" << node;
    ZigbeeReply *reply = node->readBindingTableEntries();
    connect(reply, &ZigbeeReply::finished, node, [=](){
        if (reply->error() != ZigbeeReply::ErrorNoError) {
            qCWarning(dcZigbeePhilipsHue()) << "Failed to read binding table from" << node;
        } else {
            qCDebug(dcZigbeePhilipsHue()) << "Binding table:";
            foreach (const ZigbeeDeviceProfile::BindingTableListRecord &binding, node->bindingTableRecords()) {
                qCDebug(dcZigbeePhilipsHue()) << node << binding;
            }
        }
    });
}

void IntegrationPluginZigbeePhilipsHue::initMotionSensor(ZigbeeNode *node)
{
    ZigbeeNodeEndpoint *endpointHa = node->getEndpoint(0x02);

    // Get the current configured bindings for this node
    ZigbeeReply *reply = node->removeAllBindings();
    connect(reply, &ZigbeeReply::finished, node, [=](){
        if (reply->error() != ZigbeeReply::ErrorNoError) {
            qCWarning(dcZigbeePhilipsHue()) << "Failed to remove all bindings for initialization of" << node;
        } else {
            qCDebug(dcZigbeePhilipsHue()) << "Removed all bindings successfully from" << node;
        }

        // Read battery, bind and configure attribute reporting for battery
        if (!endpointHa->hasInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)) {
            qCWarning(dcZigbeePhilipsHue()) << "Failed to initialize the power configuration cluster because the cluster could not be found" << node << endpointHa;
            return;
        }

        // Init power configuration cluster
        qCDebug(dcZigbeePhilipsHue()) << "Read power configuration cluster attributes" << node;
        ZigbeeClusterReply *readAttributeReply = endpointHa->getInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)->readAttributes({ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining});
        connect(readAttributeReply, &ZigbeeClusterReply::finished, node, [=](){
            if (readAttributeReply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigbeePhilipsHue()) << "Failed to read power cluster attributes" << readAttributeReply->error();
            } else {
                qCDebug(dcZigbeePhilipsHue()) << "Read power configuration cluster attributes finished successfully";
            }

            // Bind the cluster to the coordinator
            qCDebug(dcZigbeePhilipsHue()) << "Bind power configuration cluster to coordinator IEEE address";
            ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(endpointHa->endpointId(), ZigbeeClusterLibrary::ClusterIdPowerConfiguration,
                                                                                              hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
            connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                    qCWarning(dcZigbeePhilipsHue()) << "Failed to bind power cluster to coordinator" << zdoReply->error();
                } else {
                    qCDebug(dcZigbeePhilipsHue()) << "Bind power configuration cluster to coordinator finished successfully";
                }

                // Configure attribute rporting for battery remaining (0.5 % changes = 1)
                ZigbeeClusterLibrary::AttributeReportingConfiguration reportingConfig;
                reportingConfig.attributeId = ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining;
                reportingConfig.dataType = Zigbee::Uint8;
                reportingConfig.minReportingInterval = 300;
                reportingConfig.maxReportingInterval = 2700;
                reportingConfig.reportableChange = ZigbeeDataType(static_cast<quint8>(1)).data();

                qCDebug(dcZigbeePhilipsHue()) << "Configure attribute reporting for power configuration cluster to coordinator";
                ZigbeeClusterReply *reportingReply = endpointHa->getInputCluster(ZigbeeClusterLibrary::ClusterIdPowerConfiguration)->configureReporting({reportingConfig});
                connect(reportingReply, &ZigbeeClusterReply::finished, this, [=](){
                    if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                        qCWarning(dcZigbeePhilipsHue()) << "Failed to configure power cluster attribute reporting" << reportingReply->error();
                    } else {
                        qCDebug(dcZigbeePhilipsHue()) << "Attribute reporting configuration finished for power cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
                    }


                    // Init temperature measurment cluster
                    qCDebug(dcZigbeePhilipsHue()) << "Bind temperature cluster to coordinator IEEE address";
                    ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(endpointHa->endpointId(), ZigbeeClusterLibrary::ClusterIdTemperatureMeasurement,
                                                                                                      hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
                    connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                        if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                            qCWarning(dcZigbeePhilipsHue()) << "Failed to bind temperature cluster to coordinator" << zdoReply->error();
                        } else {
                            qCDebug(dcZigbeePhilipsHue()) << "Bind temperature cluster to coordinator finished successfully";
                        }

                        // Read current temperature
                        if (!endpointHa->hasInputCluster(ZigbeeClusterLibrary::ClusterIdTemperatureMeasurement)) {
                            qCWarning(dcZigbeePhilipsHue()) << "Failed to initialize the temperature cluster because the cluster could not be found" << node << endpointHa;
                            return;
                        }

                        qCDebug(dcZigbeePhilipsHue()) << "Read temperature cluster attributes" << node;
                        ZigbeeClusterReply *readAttributeReply = endpointHa->getInputCluster(ZigbeeClusterLibrary::ClusterIdTemperatureMeasurement)->readAttributes({ZigbeeClusterTemperatureMeasurement::AttributeMeasuredValue});
                        connect(readAttributeReply, &ZigbeeClusterReply::finished, node, [=](){
                            if (readAttributeReply->error() != ZigbeeClusterReply::ErrorNoError) {
                                qCWarning(dcZigbeePhilipsHue()) << "Failed to read temperature cluster attributes" << readAttributeReply->error();
                            } else {
                                qCDebug(dcZigbeePhilipsHue()) << "Read temperature cluster attributes finished successfully";
                            }

                            // Configure attribute reporting for temperature
                            ZigbeeClusterLibrary::AttributeReportingConfiguration reportingConfig;
                            reportingConfig.attributeId = ZigbeeClusterTemperatureMeasurement::AttributeMeasuredValue;
                            reportingConfig.dataType = Zigbee::Int16;
                            reportingConfig.minReportingInterval = 300;
                            reportingConfig.maxReportingInterval = 600;
                            reportingConfig.reportableChange = ZigbeeDataType(static_cast<qint16>(10)).data();

                            qCDebug(dcZigbeePhilipsHue()) << "Configure attribute reporting for temperature cluster to coordinator";
                            ZigbeeClusterReply *reportingReply = endpointHa->getInputCluster(ZigbeeClusterLibrary::ClusterIdTemperatureMeasurement)->configureReporting({reportingConfig});
                            connect(reportingReply, &ZigbeeClusterReply::finished, this, [=](){
                                if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                                    qCWarning(dcZigbeePhilipsHue()) << "Failed to configure temperature cluster attribute reporting" << reportingReply->error();
                                } else {
                                    qCDebug(dcZigbeePhilipsHue()) << "Attribute reporting configuration finished for temperature cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
                                }

                                // Init illuminance measurment cluster
                                qCDebug(dcZigbeePhilipsHue()) << "Bind illuminance measurement cluster to coordinator IEEE address";
                                ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(endpointHa->endpointId(), ZigbeeClusterLibrary::ClusterIdIlluminanceMeasurement,
                                                                                                                  hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
                                connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                                    if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                                        qCWarning(dcZigbeePhilipsHue()) << "Failed to bind illuminance measurement cluster to coordinator" << zdoReply->error();
                                    } else {
                                        qCDebug(dcZigbeePhilipsHue()) << "Bind illuminance measurement cluster to coordinator finished successfully";
                                    }

                                    // Read current illuminance
                                    if (!endpointHa->hasInputCluster(ZigbeeClusterLibrary::ClusterIdIlluminanceMeasurement)) {
                                        qCWarning(dcZigbeePhilipsHue()) << "Failed to initialize the illuminance measurement cluster because the cluster could not be found" << node << endpointHa;
                                        return;
                                    }

                                    qCDebug(dcZigbeePhilipsHue()) << "Read illuminance measurement cluster attributes" << node;
                                    ZigbeeClusterReply *readAttributeReply = endpointHa->getInputCluster(ZigbeeClusterLibrary::ClusterIdIlluminanceMeasurement)->readAttributes({ZigbeeClusterIlluminanceMeasurement::AttributeMeasuredValue});
                                    connect(readAttributeReply, &ZigbeeClusterReply::finished, node, [=](){
                                        if (readAttributeReply->error() != ZigbeeClusterReply::ErrorNoError) {
                                            qCWarning(dcZigbeePhilipsHue()) << "Failed to read illuminance measurement cluster attributes" << readAttributeReply->error();
                                        } else {
                                            qCDebug(dcZigbeePhilipsHue()) << "Read illuminance measurement cluster attributes finished successfully";
                                        }

                                        // Configure attribute reporting for illuminance
                                        ZigbeeClusterLibrary::AttributeReportingConfiguration reportingConfig;
                                        reportingConfig.attributeId = ZigbeeClusterIlluminanceMeasurement::AttributeMeasuredValue;
                                        reportingConfig.dataType = Zigbee::Uint16;
                                        reportingConfig.minReportingInterval = 5;
                                        reportingConfig.maxReportingInterval = 600;
                                        reportingConfig.reportableChange = ZigbeeDataType(static_cast<quint16>(100)).data();

                                        qCDebug(dcZigbeePhilipsHue()) << "Configure attribute reporting for illuminance measurement cluster to coordinator";
                                        ZigbeeClusterReply *reportingReply = endpointHa->getInputCluster(ZigbeeClusterLibrary::ClusterIdIlluminanceMeasurement)->configureReporting({reportingConfig});
                                        connect(reportingReply, &ZigbeeClusterReply::finished, this, [=](){
                                            if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                                                qCWarning(dcZigbeePhilipsHue()) << "Failed to configure illuminance measurement cluster attribute reporting" << reportingReply->error();
                                            } else {
                                                qCDebug(dcZigbeePhilipsHue()) << "Attribute reporting configuration finished for illuminance measurement cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
                                            }


                                            // Init occupancy sensing cluster
                                            qCDebug(dcZigbeePhilipsHue()) << "Bind occupancy sensing cluster to coordinator IEEE address";
                                            ZigbeeDeviceObjectReply * zdoReply = node->deviceObject()->requestBindIeeeAddress(endpointHa->endpointId(), ZigbeeClusterLibrary::ClusterIdOccupancySensing,
                                                                                                                              hardwareManager()->zigbeeResource()->coordinatorAddress(node->networkUuid()), 0x01);
                                            connect(zdoReply, &ZigbeeDeviceObjectReply::finished, node, [=](){
                                                if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
                                                    qCWarning(dcZigbeePhilipsHue()) << "Failed to bind occupancy sensing cluster to coordinator" << zdoReply->error();
                                                } else {
                                                    qCDebug(dcZigbeePhilipsHue()) << "Bind occupancy sensing cluster to coordinator finished successfully";
                                                }


                                                qCDebug(dcZigbeePhilipsHue()) << "Read occupancy sensing cluster attributes" << node;
                                                ZigbeeClusterReply *readAttributeReply = endpointHa->getInputCluster(ZigbeeClusterLibrary::ClusterIdOccupancySensing)->readAttributes({ZigbeeClusterOccupancySensing::AttributeOccupancy, ZigbeeClusterOccupancySensing::AttributeOccupancySensorType});
                                                connect(readAttributeReply, &ZigbeeClusterReply::finished, node, [=](){
                                                    if (readAttributeReply->error() != ZigbeeClusterReply::ErrorNoError) {
                                                        qCWarning(dcZigbeePhilipsHue()) << "Failed to read occupancy sensing cluster attributes" << readAttributeReply->error();
                                                    } else {
                                                        qCDebug(dcZigbeePhilipsHue()) << "Read occupancy sensing cluster attributes finished successfully";
                                                    }

                                                    // Configure attribute reporting for temperature
                                                    ZigbeeClusterLibrary::AttributeReportingConfiguration reportingConfig;
                                                    reportingConfig.attributeId = ZigbeeClusterOccupancySensing::AttributeOccupancy;
                                                    reportingConfig.dataType = Zigbee::BitMap8;
                                                    reportingConfig.minReportingInterval = 0;
                                                    reportingConfig.maxReportingInterval = 300;

                                                    qCDebug(dcZigbeePhilipsHue()) << "Configure attribute reporting for occupancy cluster to coordinator";
                                                    ZigbeeClusterReply *reportingReply = endpointHa->getInputCluster(ZigbeeClusterLibrary::ClusterIdOccupancySensing)->configureReporting({reportingConfig});
                                                    connect(reportingReply, &ZigbeeClusterReply::finished, this, [=](){
                                                        if (reportingReply->error() != ZigbeeClusterReply::ErrorNoError) {
                                                            qCWarning(dcZigbeePhilipsHue()) << "Failed to configure occupancy cluster attribute reporting" << reportingReply->error();
                                                        } else {
                                                            qCDebug(dcZigbeePhilipsHue()) << "Attribute reporting configuration finished for occupancy cluster" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reportingReply->responseFrame().payload);
                                                        }
                                                    });
                                                });
                                            });
                                        });
                                    });
                                });
                            });
                        });
                    });
                });
            });
        });
    });
}

