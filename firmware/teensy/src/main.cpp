/*
 * C4I Telemetry Hub - Complete Implementation
 * Multi-vehicle tracking and command routing
 */

#include <Arduino.h>
#include "config.h"
#include "vehicle_manager.h"

#define LED_STATUS      13
#define LED_RX          2
#define LED_TX          3
#define LED_ERROR       4

VehicleManager vehicleManager;

static uint32_t last_status_ms = 0;
static uint32_t last_heartbeat_ms = 0;
static uint32_t rx_packets = 0;
static uint32_t tx_packets = 0;
static uint32_t parse_errors = 0;

static char radio1_buf[256];
static uint16_t radio1_idx = 0;
static char radio2_buf[256];
static uint16_t radio2_idx = 0;
static char usb_buf[256];
static uint16_t usb_idx = 0;

static bool parse_telemetry(const char* line, VehicleTelemetry* tel) {
    if (strncmp(line, "TEL:", 4) != 0) return false;
    
    int id, type, state, sats;
    double lat, lon;
    float alt, hdg, spd, bat;
    
    int parsed = sscanf(line + 4, "%d,%d,%d,%lf,%lf,%f,%f,%f,%f,%d",
                       &id, &type, &state, &lat, &lon, &alt, &hdg, &spd, &bat, &sats);
    
    if (parsed != 10) return false;
    
    tel->id = id;
    tel->type = static_cast<VehicleType>(type);
    tel->state = static_cast<VehicleState>(state);
    tel->lat = lat;
    tel->lon = lon;
    tel->alt = alt;
    tel->heading = hdg;
    tel->speed = spd;
    tel->battery = bat;
    tel->satellites = sats;
    tel->valid = true;
    
    return true;
}

static void process_radio_line(const char* line, uint8_t source) {
    VehicleTelemetry tel;
    if (parse_telemetry(line, &tel)) {
        vehicleManager.updateTelemetry(tel.id, tel);
        rx_packets++;
        digitalWrite(LED_RX, HIGH);
    } else {
        parse_errors++;
    }
}

static void process_gcs_command(const char* line) {
    if (strncmp(line, "CMD:", 4) != 0) return;
    
    Command cmd;
    int target, type;
    float p1, p2, p3;
    
    if (sscanf(line + 4, "%d,%d,%f,%f,%f", &target, &type, &p1, &p2, &p3) >= 2) {
        cmd.targetId = target;
        cmd.cmdType = type;
        cmd.param1 = p1;
        cmd.param2 = p2;
        cmd.param3 = p3;
        cmd.timestamp = millis();
        
        if (target == 0) {
            vehicleManager.broadcastCommand(cmd);
        } else {
            vehicleManager.sendCommand(target, cmd);
        }
        
        tx_packets++;
        digitalWrite(LED_TX, HIGH);
    }
}

static void read_serial_line(Stream& serial, char* buf, uint16_t& idx, 
                             uint16_t maxLen, void (*handler)(const char*, uint8_t), uint8_t source) {
    while (serial.available()) {
        char c = serial.read();
        if (c == '\n' || c == '\r') {
            if (idx > 0) {
                buf[idx] = 0;
                handler(buf, source);
                idx = 0;
            }
        } else if (idx < maxLen - 1) {
            buf[idx++] = c;
        }
    }
}

static void send_status_json(void) {
    Serial.print("{\"type\":\"status\",\"ts\":");
    Serial.print(millis());
    Serial.print(",\"active\":");
    Serial.print(vehicleManager.getActiveCount());
    Serial.print(",\"rx\":");
    Serial.print(rx_packets);
    Serial.print(",\"tx\":");
    Serial.print(tx_packets);
    Serial.print(",\"err\":");
    Serial.print(parse_errors);
    Serial.print(",\"vehicles\":[");
    
    bool first = true;
    for (uint8_t i = 1; i <= Config::MAX_VEHICLES; i++) {
        VehicleTelemetry* v = vehicleManager.getVehicle(i);
        if (v && v->valid) {
            if (!first) Serial.print(",");
            first = false;
            
            Serial.print("{\"id\":");
            Serial.print(v->id);
            Serial.print(",\"type\":");
            Serial.print((int)v->type);
            Serial.print(",\"state\":");
            Serial.print((int)v->state);
            Serial.print(",\"lat\":");
            Serial.print(v->lat, 7);
            Serial.print(",\"lon\":");
            Serial.print(v->lon, 7);
            Serial.print(",\"alt\":");
            Serial.print(v->alt, 1);
            Serial.print(",\"hdg\":");
            Serial.print(v->heading, 1);
            Serial.print(",\"spd\":");
            Serial.print(v->speed, 1);
            Serial.print(",\"bat\":");
            Serial.print(v->battery, 0);
            Serial.print(",\"sats\":");
            Serial.print(v->satellites);
            Serial.print(",\"age\":");
            Serial.print(millis() - v->lastUpdate);
            Serial.print("}");
        }
    }
    
    Serial.println("]}");
}

static void send_heartbeat(void) {
    char buf[64];
    snprintf(buf, sizeof(buf), "HB:%lu,%d,%lu,%lu\n",
             millis(), vehicleManager.getActiveCount(), rx_packets, tx_packets);
    
    Serial1.print(buf);
    Serial2.print(buf);
}

void setup() {
    Serial.begin(Config::Serial::USB_BAUD);
    Serial1.begin(Config::Serial::RADIO1_BAUD);
    Serial2.begin(Config::Serial::RADIO2_BAUD);
    
    pinMode(LED_STATUS, OUTPUT);
    pinMode(LED_RX, OUTPUT);
    pinMode(LED_TX, OUTPUT);
    pinMode(LED_ERROR, OUTPUT);
    
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_STATUS, HIGH);
        digitalWrite(LED_RX, HIGH);
        digitalWrite(LED_TX, HIGH);
        delay(100);
        digitalWrite(LED_STATUS, LOW);
        digitalWrite(LED_RX, LOW);
        digitalWrite(LED_TX, LOW);
        delay(100);
    }
    
    vehicleManager.init();
    
    Serial.println("{\"type\":\"init\",\"status\":\"ready\",\"version\":\"1.0.0\"}");
    
    last_status_ms = millis();
    last_heartbeat_ms = millis();
}

void loop() {
    uint32_t now = millis();
    
    read_serial_line(Serial1, radio1_buf, radio1_idx, sizeof(radio1_buf), process_radio_line, 1);
    read_serial_line(Serial2, radio2_buf, radio2_idx, sizeof(radio2_buf), process_radio_line, 2);
    
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (usb_idx > 0) {
                usb_buf[usb_idx] = 0;
                process_gcs_command(usb_buf);
                usb_idx = 0;
            }
        } else if (usb_idx < sizeof(usb_buf) - 1) {
            usb_buf[usb_idx++] = c;
        }
    }
    
    vehicleManager.update();
    
    if (now - last_status_ms >= Config::STATUS_RATE_MS) {
        last_status_ms = now;
        send_status_json();
    }
    
    if (now - last_heartbeat_ms >= 5000) {
        last_heartbeat_ms = now;
        send_heartbeat();
        digitalToggle(LED_STATUS);
    }
    
    static uint32_t led_off_time = 0;
    if (led_off_time > 0 && now > led_off_time) {
        digitalWrite(LED_RX, LOW);
        digitalWrite(LED_TX, LOW);
        led_off_time = 0;
    }
    if (digitalRead(LED_RX) || digitalRead(LED_TX)) {
        led_off_time = now + 50;
    }
    
    if (parse_errors > 0 && (now % 1000) < 100) {
        digitalWrite(LED_ERROR, HIGH);
    } else {
        digitalWrite(LED_ERROR, LOW);
    }
}
