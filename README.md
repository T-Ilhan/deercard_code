# DeerPlatformESP

Tank-drive robot platform controlled over BLE using a gamepad, with an ESP32 firmware backend and a Python controller front end.

## Architecture

```
Gamepad (pygame) -> controller.py -> BLE (bleak) -> ESP32 (main.cpp, logging.cpp)
```

`controller.py` reads gamepad axes, converts them into a single-character drive command, and streams it over BLE via the Nordic UART Service (NUS). `main.cpp` on the ESP32 receives the character and drives two BTS7960 motor controllers via MCPWM. `logging.cpp` provides color-coded serial logging (debug, ok, info, warn, err).

## Command Set

| Char | Action |
|---|---|
| `w` | Forward |
| `s` | Backward |
| `a` | Turn left (point turn, tuned by `TURN_AGGRESSION`) |
| `d` | Turn right |
| `q` | Forward-left arc (wide) |
| `e` | Forward-right arc (wide) |
| ` ` | Stop |

Commands are derived from left/right stick Y-axis input in `axes_to_cmd()`.

## Firmware (main.cpp)

- BLE service: Nordic UART (`NUS_SERVICE_UUID` / RX / TX characteristics)
- Motors: two BTS7960 drivers via `ESP32_MCPWM`, 10kHz PWM
  - Left: pins 4/5 (PWM), 6/7 (enable)
  - Right: pins 9/10 (PWM), 11/12 (enable)
- `DRIVE_SPEED` (default 70%) and `TURN_AGGRESSION` (0 to 1) tune straight-line speed and point-turn tightness
- Safety timeout: motors stop if no command received for `CMD_TIMEOUT_MS` (150ms)
- Commented-out alternate: WiFi/UDP version of the same control scheme, same command set, `CMD_TIMEOUT_MS` bumped to 500ms

## Controller (controller.py)

- Connects to ESP32 at `DEVICE_ADDR`, writes to `NUS_RX_UUID` at `SEND_INTERVAL` (80ms)
- Reads gamepad axis 1 (left stick Y) and axis 3 (right stick Y)
- `DEADZONE` (0.2) filters stick noise
- Auto-reconnects on BLE disconnect
- Runs BLE loop on a background thread, gamepad polling on main thread
- Commented-out alternates in the same file:
  - **Legacy variable-speed mode**: sends `"L+0.75 R-0.50\n"` style strings for proportional (not just on/off) motor speed, with arcade-style forward/turn mixing
  - **WiFi/UDP mode**: same command set sent over UDP to `192.168.4.1:4210` instead of BLE

## Logging (logging.cpp)

ANSI color-coded serial output by level: DEBUG (gray), OK (green), INFO (cyan), WARN (yellow), ERR (red). Used throughout `main.cpp` for BLE connection state and drive commands.

## Setup

### Firmware
```
pio run --target upload
```
Confirm board target in `platformio.ini` first.

### Controller
```
pip install bleak pygame
python controller.py
```
Update `DEVICE_ADDR` to match your ESP32's BLE MAC address before running.

## Status

BLE tank drive is the active mode in both `main.cpp` and `controller.py`. Variable-speed BLE and WiFi/UDP control exist as commented-out alternates in both files, not currently wired in.