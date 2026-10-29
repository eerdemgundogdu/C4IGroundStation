# C4IGroundStation

Multi-vehicle telemetry aggregator on Teensy 4.1 and PyQt6 ground station interface.

## Structure

- `firmware/teensy/`: Teensy 4.1 packet router and telemetry hub (`main.cpp`, `vehicle_manager.cpp`). Polls vehicle links, checks link timeouts and forwards combined status over USB serial.
- `gcs/`: Python ground control interface (`gcs/src/main.py`).

## Build and Run

```bash
cd firmware/teensy
pio run

cd ../../gcs
pip install -r requirements.txt
python src/main.py
```
