#pragma once

#include "config.h"

class VehicleManager {
public:
    void init();
    void update();
    bool addVehicle(uint8_t id, VehicleType type);
    bool removeVehicle(uint8_t id);
    void updateTelemetry(uint8_t id, const VehicleTelemetry& telemetry);
    VehicleTelemetry* getVehicle(uint8_t id);
    uint8_t getActiveCount() const;
    void checkTimeouts();
    void broadcastCommand(const Command& cmd);
    void sendCommand(uint8_t vehicleId, const Command& cmd);
    
private:
    VehicleTelemetry vehicles[Config::MAX_VEHICLES];
    uint8_t activeCount = 0;
    
    int findVehicleIndex(uint8_t id) const;
};
