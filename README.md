# C4I Ground Control Station

A Command, Control, Communications, Computers and Intelligence (C4I) ground control station demonstrating multi-vehicle coordination, real-time telemetry, and mission planning capabilities.

![Status](https://img.shields.io/badge/status-development-orange)
![Platform](https://img.shields.io/badge/platform-Jetson%20Nano%20%7C%20Teensy%204.1-blue)
![License](https://img.shields.io/badge/license-MIT-green)

## Overview

This project implements a ground control station for coordinating multiple unmanned vehicles, featuring real-time telemetry display, mission planning, and tactical decision support. Designed to demonstrate C4I competencies for defense applications.

### Target Applications
- Multi-vehicle command and control
- UAV/UGV fleet coordination
- Mission planning and execution
- Real-time situational awareness

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                   C4I GROUND CONTROL STATION                     │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────────────┐         ┌──────────────────┐              │
│  │   Jetson Nano    │  USB    │   Teensy 4.1     │              │
│  │  Command Center  │◄───────►│  Telemetry Hub   │              │
│  │                  │         │                   │              │
│  │  • PyQt6 GUI     │         │  • Multi-Radio   │              │
│  │  • Mission Plan  │         │  • Data Fusion   │              │
│  │  • Map Display   │         │  • Protocol Conv │              │
│  └────────┬─────────┘         └────────┬──────────┘              │
│           │                            │                         │
│           ▼                            ▼                         │
│  ┌─────────────────────────────────────────────────────┐        │
│  │              Vehicle Links                           │        │
│  │  [UAV-1] [UAV-2] [UGV-1] [UGV-2] [Sensor-1]         │        │
│  └─────────────────────────────────────────────────────┘        │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

## Features

### Command Center (Jetson Nano)
- **PyQt6 Dashboard**: Tactical display interface
- **Mission Planning**: Waypoint and task assignment
- **Map Integration**: Real-time vehicle tracking
- **Decision Support**: Threat assessment display

### Telemetry Hub (Teensy 4.1)
- **Multi-Vehicle**: Up to 8 simultaneous links
- **Protocol Bridge**: MAVLink, custom protocols
- **Data Fusion**: Combined situation picture
- **Failover**: Automatic backup routing

### Communication
- **Radio Links**: 900MHz / 2.4GHz telemetry
- **Ethernet**: Wired backup connection
- **Data Rates**: 57.6 - 921.6 kbps

## Hardware Requirements

| Component | Specification |
|-----------|---------------|
| Jetson Nano | 4GB RAM, Ubuntu 18.04+ |
| Teensy 4.1 | ARM Cortex-M7, 600 MHz |
| Display | 1920x1080 HDMI |
| Radio | SiK/RFD900 telemetry |

## Project Structure

```
C4IGroundStation/
├── README.md
├── docs/
│   └── user_manual.md
├── gcs/
│   ├── src/
│   ├── ui/
│   └── requirements.txt
├── firmware/
│   └── teensy/
│       ├── src/
│       ├── include/
│       └── platformio.ini
└── common/
    └── protocols/
```

## Dashboard Features

```
┌───────────────────────────────────────────────────────────┐
│ C4I Ground Control Station          [▼] [□] [×]           │
├───────────────────────────────────────────────────────────┤
│ ┌───────────────────────────┐ ┌─────────────────────────┐ │
│ │                           │ │ Vehicle Status          │ │
│ │                           │ │ ┌─────┬───────┬───────┐ │ │
│ │      MAP DISPLAY          │ │ │ ID  │ State │ Batt  │ │ │
│ │     (OpenStreetMap)       │ │ ├─────┼───────┼───────┤ │ │
│ │                           │ │ │UAV-1│ Active│ 85%   │ │ │
│ │  [UAV-1]●  [UAV-2]●       │ │ │UAV-2│ RTL   │ 42%   │ │ │
│ │                           │ │ │UGV-1│ Hold  │ 67%   │ │ │
│ │       [UGV-1]■            │ │ └─────┴───────┴───────┘ │ │
│ └───────────────────────────┘ └─────────────────────────┘ │
│ ┌───────────────────────────┐ ┌─────────────────────────┐ │
│ │ Mission Log               │ │ Commands                │ │
│ │ 14:32:15 UAV-1 WP reached │ │ [Start] [Stop] [RTL]    │ │
│ │ 14:32:18 UGV-1 Target det │ │ [Add WP] [Clear]        │ │
│ └───────────────────────────┘ └─────────────────────────┘ │
└───────────────────────────────────────────────────────────┘
```

## Author

**Cihan Erdem Gündoğdu** - Embedded Systems Engineer

