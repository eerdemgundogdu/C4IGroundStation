#pragma once

#include <stdint.h>

namespace Config {
    constexpr uint8_t MAX_VEHICLES = 8;
    constexpr uint32_t HEARTBEAT_TIMEOUT_MS = 3000;
    constexpr uint32_t TELEMETRY_RATE_MS = 100;
    constexpr uint32_t STATUS_RATE_MS = 1000;
    
    namespace Serial {
        constexpr uint32_t USB_BAUD = 115200;
        constexpr uint32_t RADIO1_BAUD = 57600;
        constexpr uint32_t RADIO2_BAUD = 57600;
    }
    
    namespace Pins {
        constexpr uint8_t LED_STATUS = 13;
        constexpr uint8_t LED_RX = 2;
        constexpr uint8_t LED_TX = 3;
        constexpr uint8_t RADIO1_TX = 1;
        constexpr uint8_t RADIO1_RX = 0;
        constexpr uint8_t RADIO2_TX = 8;
        constexpr uint8_t RADIO2_RX = 7;
    }
}

enum class VehicleType : uint8_t {
    UNKNOWN = 0,
    QUADROTOR,
    FIXED_WING,
    GROUND_VEHICLE,
    BOAT,
    SENSOR_NODE
};

enum class VehicleState : uint8_t {
    OFFLINE = 0,
    STANDBY,
    ACTIVE,
    RTL,
    EMERGENCY,
    LOST
};

struct VehicleTelemetry {
    uint8_t id;
    VehicleType type;
    VehicleState state;
    double lat, lon;
    float alt;
    float heading;
    float speed;
    float battery;
    uint8_t satellites;
    uint32_t lastUpdate;
    bool valid;
};

struct MissionWaypoint {
    uint8_t seq;
    double lat, lon;
    float alt;
    uint8_t command;
    float param1, param2;
};

struct Command {
    uint8_t targetId;
    uint8_t cmdType;
    float param1, param2, param3;
    uint32_t timestamp;
};
