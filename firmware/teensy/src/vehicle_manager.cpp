#include "vehicle_manager.h"
#include <Arduino.h>

void VehicleManager::init() {
    for (uint8_t i = 0; i < Config::MAX_VEHICLES; i++) {
        vehicles[i].valid = false;
        vehicles[i].id = 0;
        vehicles[i].state = VehicleState::OFFLINE;
    }
    activeCount = 0;
}

void VehicleManager::update() {
    checkTimeouts();
}

int VehicleManager::findVehicleIndex(uint8_t id) const {
    for (uint8_t i = 0; i < Config::MAX_VEHICLES; i++) {
        if (vehicles[i].valid && vehicles[i].id == id) {
            return i;
        }
    }
    return -1;
}

bool VehicleManager::addVehicle(uint8_t id, VehicleType type) {
    if (findVehicleIndex(id) >= 0) return false;
    
    for (uint8_t i = 0; i < Config::MAX_VEHICLES; i++) {
        if (!vehicles[i].valid) {
            vehicles[i].id = id;
            vehicles[i].type = type;
            vehicles[i].state = VehicleState::STANDBY;
            vehicles[i].valid = true;
            vehicles[i].lastUpdate = millis();
            activeCount++;
            return true;
        }
    }
    return false;
}

bool VehicleManager::removeVehicle(uint8_t id) {
    int idx = findVehicleIndex(id);
    if (idx < 0) return false;
    
    vehicles[idx].valid = false;
    vehicles[idx].state = VehicleState::OFFLINE;
    activeCount--;
    return true;
}

void VehicleManager::updateTelemetry(uint8_t id, const VehicleTelemetry& telemetry) {
    int idx = findVehicleIndex(id);
    if (idx < 0) {
        addVehicle(id, telemetry.type);
        idx = findVehicleIndex(id);
    }
    
    if (idx >= 0) {
        vehicles[idx] = telemetry;
        vehicles[idx].lastUpdate = millis();
        vehicles[idx].valid = true;
        
        if (vehicles[idx].state == VehicleState::LOST) {
            vehicles[idx].state = VehicleState::ACTIVE;
        }
    }
}

VehicleTelemetry* VehicleManager::getVehicle(uint8_t id) {
    int idx = findVehicleIndex(id);
    return (idx >= 0) ? &vehicles[idx] : nullptr;
}

uint8_t VehicleManager::getActiveCount() const {
    return activeCount;
}

void VehicleManager::checkTimeouts() {
    uint32_t now = millis();
    for (uint8_t i = 0; i < Config::MAX_VEHICLES; i++) {
        if (vehicles[i].valid) {
            uint32_t elapsed = now - vehicles[i].lastUpdate;
            if (elapsed > Config::HEARTBEAT_TIMEOUT_MS) {
                if (vehicles[i].state != VehicleState::LOST) {
                    vehicles[i].state = VehicleState::LOST;
                }
            }
        }
    }
}

void VehicleManager::broadcastCommand(const Command& cmd) {
    for (uint8_t i = 0; i < Config::MAX_VEHICLES; i++) {
        if (vehicles[i].valid && vehicles[i].state != VehicleState::OFFLINE) {
            sendCommand(vehicles[i].id, cmd);
        }
    }
}

void VehicleManager::sendCommand(uint8_t vehicleId, const Command& cmd) {
    char buf[64];
    snprintf(buf, sizeof(buf), "CMD:%d,%d,%.2f,%.2f,%.2f\n",
             vehicleId, cmd.cmdType, cmd.param1, cmd.param2, cmd.param3);
    Serial1.print(buf);
}
