
// THE TANK DRIVE OLD MAIN WITH THE SHROT RxCAll backs

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ESP32_MCPWM.h>
#include "logging.h"

#define NUS_SERVICE_UUID    "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_RX_CHAR_UUID    "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_TX_CHAR_UUID    "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define BOOT_DELAY      1000
#define PWM_FREQ        10000
#define CMD_TIMEOUT_MS  150

// ---- Tuning ----
#define DRIVE_SPEED     70.0f   // % throttle when driving straight (0-100)
#define TURN_AGGRESSION 0.8f    // 0.0 = gentle arc, 1.0 = full pivot on spot

// Derived turn speeds from aggression parameter:
// Outer wheel always at DRIVE_SPEED.
// Inner wheel goes from DRIVE_SPEED (aggression=0) down to -DRIVE_SPEED (aggression=1).
// At aggression=0.5, inner wheel is 0 (pure point turn).
// At aggression=1.0, inner wheel is full reverse (tightest spin).
static float turn_outer_speed() { return DRIVE_SPEED; }
static float turn_inner_speed() { return DRIVE_SPEED * (1.0f - 2.0f * TURN_AGGRESSION); }

// ---- Left motor pins (BTS7960) ----
#define L_LPWM_PIN  5
#define L_RPWM_PIN  4
#define L_LEN_PIN   7
#define L_REN_PIN   6

// ---- Right motor pins (BTS7960) ----
#define R_LPWM_PIN  10
#define R_RPWM_PIN  9
#define R_LEN_PIN   11
#define R_REN_PIN   12

int MOTOR_ENABLE_PINS[4] = { L_LEN_PIN, L_REN_PIN, R_LEN_PIN, R_REN_PIN };

MotorMCPWMConfig L_cfg { L_LPWM_PIN, L_RPWM_PIN, -1, MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM0A, MCPWM0B };
MotorMCPWMConfig R_cfg { R_LPWM_PIN, R_RPWM_PIN, -1, MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM1A, MCPWM1B };

Motor motor_L, motor_R;

static constexpr Dir L_FWD = Dir::CW;
static constexpr Dir R_FWD = Dir::CCW;

static unsigned long last_cmd_ms = 0;
static bool ble_connected = false;

// ---- Motor helpers ----

void setMotor(Motor& motor, Dir fwd, float speed_pct) {
    if (speed_pct >= 0.0f) {
        motor.setSpeedPercent(speed_pct, fwd);
    } else {
        motor.setSpeedPercent(-speed_pct, IMotorDriver::changeDir(fwd));
    }
}

void stopMotors() {
    motor_L.setSpeed(0, L_FWD);
    motor_R.setSpeed(0, R_FWD);
}

void driveForward() {
    setMotor(motor_L, L_FWD, DRIVE_SPEED);
    setMotor(motor_R, R_FWD, DRIVE_SPEED);
}

void driveBackward() {
    setMotor(motor_L, L_FWD, -DRIVE_SPEED);
    setMotor(motor_R, R_FWD, -DRIVE_SPEED);
}

void turnLeft() {
    // Right wheel = inner, Left wheel = outer
    setMotor(motor_L, L_FWD, turn_outer_speed());
    setMotor(motor_R, R_FWD, turn_inner_speed());
}

void turnRight() {
    // Left wheel = inner, Right wheel = outer
    setMotor(motor_L, L_FWD, turn_inner_speed());
    setMotor(motor_R, R_FWD, turn_outer_speed());
}

void forwardLeft() {
    // Both wheels forward, left slower -- wide arc forward-left
    setMotor(motor_L, L_FWD, DRIVE_SPEED * 0.4f);
    setMotor(motor_R, R_FWD, DRIVE_SPEED);
}

void forwardRight() {
    // Both wheels forward, right slower -- wide arc forward-right
    setMotor(motor_L, L_FWD, DRIVE_SPEED);
    setMotor(motor_R, R_FWD, DRIVE_SPEED * 0.4f);
}

// ---- BLE callbacks ----

class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* server) override {
        ble_connected = true;
        last_cmd_ms = millis();
        LOG_OK("BLE", "Client connected");
    }
    void onDisconnect(BLEServer* server) override {
        ble_connected = false;
        stopMotors();
        LOG_WARN("BLE", "Client disconnected, restarting advertising...");
        BLEDevice::startAdvertising();
    }
};

class RxCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* characteristic) override {
        std::string data = characteristic->getValue();
        if (data.empty()) return;

        char cmd = data[0];
        last_cmd_ms = millis();

        switch (cmd) {
            case 'w': driveForward();  LOG_DEBUG("BLE", "Forward");       break;
            case 'q': forwardLeft();   LOG_DEBUG("BLE", "Forward-Left");  break;
            case 'e': forwardRight();  LOG_DEBUG("BLE", "Forward-Right"); break;
            case 's': driveBackward(); LOG_DEBUG("BLE", "Backward"); break;
            case 'a': turnLeft();      LOG_DEBUG("BLE", "Left");     break;
            case 'd': turnRight();     LOG_DEBUG("BLE", "Right");    break;
            case ' ': stopMotors();    LOG_DEBUG("BLE", "Stop");     break;
            default: break;
        }
    }
};

// ---- Setup ----

void setup() {
    delay(BOOT_DELAY);
    Serial.begin(115200);
    LOG_INFO("DeerPlatformESP", "Starting BLE mode...");
    LOG_INFO("DeerPlatformESP", "Drive speed: %.0f%%, Turn aggression: %.2f", DRIVE_SPEED, TURN_AGGRESSION);
    LOG_INFO("DeerPlatformESP", "Turn outer: %.0f%%, inner: %.0f%%", turn_outer_speed(), turn_inner_speed());

    for (int pin : MOTOR_ENABLE_PINS) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, HIGH);
    }

    L_cfg.pwm_freq_hz = PWM_FREQ;
    motor_L.setup(L_cfg);
    motor_L.start();

    R_cfg.pwm_freq_hz = PWM_FREQ;
    motor_R.setup(R_cfg);
    motor_R.start();

    stopMotors();

    BLEDevice::init("DeerPlatform");
    BLEServer* server = BLEDevice::createServer();
    server->setCallbacks(new ServerCallbacks());

    BLEService* service = server->createService(NUS_SERVICE_UUID);

    BLECharacteristic* rxChar = service->createCharacteristic(
        NUS_RX_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );
    rxChar->setCallbacks(new RxCallbacks());

    BLECharacteristic* txChar = service->createCharacteristic(
        NUS_TX_CHAR_UUID,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    txChar->addDescriptor(new BLE2902());

    service->start();

    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(NUS_SERVICE_UUID);
    advertising->setScanResponse(true);
    BLEDevice::startAdvertising();

    LOG_OK("BLE", "Advertising as 'DeerPlatform' -- waiting for connection");
}

// ---- Loop ----

void loop() {
    if (ble_connected && millis() - last_cmd_ms > CMD_TIMEOUT_MS) {
        stopMotors();
    }
    delay(10);
}







// #include <Arduino.h>
// #include <WiFi.h>
// #include <WiFiUDP.h>
// #include <ESP32_MCPWM.h>
// #include "logging.h"

// // ---- WiFi AP config ----
// #define WIFI_SSID       "Tulgar's iPhone"
// #define WIFI_PASS       "Mersin3334"     // min 8 chars, change if you want
// #define UDP_PORT        4210

// // ---- Tuning ----
// #define DRIVE_SPEED     80.0f
// #define TURN_AGGRESSION 0.8f
// #define CMD_TIMEOUT_MS  500            // bumped from 150

// // ---- Motor pins ----
// #define L_LPWM_PIN  5
// #define L_RPWM_PIN  4
// #define L_LEN_PIN   7
// #define L_REN_PIN   6
// #define R_LPWM_PIN  10
// #define R_RPWM_PIN  9
// #define R_LEN_PIN   11
// #define R_REN_PIN   12

// int MOTOR_ENABLE_PINS[4] = { L_LEN_PIN, L_REN_PIN, R_LEN_PIN, R_REN_PIN };

// MotorMCPWMConfig L_cfg { L_LPWM_PIN, L_RPWM_PIN, -1, MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM0A, MCPWM0B };
// MotorMCPWMConfig R_cfg { R_LPWM_PIN, R_RPWM_PIN, -1, MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM1A, MCPWM1B };

// Motor motor_L, motor_R;

// static constexpr Dir L_FWD = Dir::CW;
// static constexpr Dir R_FWD = Dir::CCW;

// static unsigned long last_cmd_ms = 0;

// WiFiUDP udp;

// // ---- Motor helpers (identical to before) ----

// static float turn_outer_speed() { return DRIVE_SPEED; }
// static float turn_inner_speed() { return DRIVE_SPEED * (1.0f - 2.0f * TURN_AGGRESSION); }

// void setMotor(Motor& motor, Dir fwd, float speed_pct) {
//     if (speed_pct >= 0.0f)
//         motor.setSpeedPercent(speed_pct, fwd);
//     else
//         motor.setSpeedPercent(-speed_pct, IMotorDriver::changeDir(fwd));
// }

// void stopMotors()    { motor_L.setSpeed(0, L_FWD); motor_R.setSpeed(0, R_FWD); }
// void driveForward()  { setMotor(motor_L, L_FWD, DRIVE_SPEED);  setMotor(motor_R, R_FWD, DRIVE_SPEED); }
// void driveBackward() { setMotor(motor_L, L_FWD, -DRIVE_SPEED); setMotor(motor_R, R_FWD, -DRIVE_SPEED); }
// void turnLeft()      { setMotor(motor_L, L_FWD, turn_outer_speed()); setMotor(motor_R, R_FWD, turn_inner_speed()); }
// void turnRight()     { setMotor(motor_L, L_FWD, turn_inner_speed()); setMotor(motor_R, R_FWD, turn_outer_speed()); }
// void forwardLeft()   { setMotor(motor_L, L_FWD, DRIVE_SPEED * 0.4f); setMotor(motor_R, R_FWD, DRIVE_SPEED); }
// void forwardRight()  { setMotor(motor_L, L_FWD, DRIVE_SPEED); setMotor(motor_R, R_FWD, DRIVE_SPEED * 0.4f); }

// void handleCmd(char cmd) {
//     last_cmd_ms = millis();
//     switch (cmd) {
//         case 'w': driveForward();  LOG_DEBUG("UDP", "Forward");       break;
//         case 'q': forwardLeft();   LOG_DEBUG("UDP", "Forward-Left");  break;
//         case 'e': forwardRight();  LOG_DEBUG("UDP", "Forward-Right"); break;
//         case 's': driveBackward(); LOG_DEBUG("UDP", "Backward");      break;
//         case 'a': turnLeft();      LOG_DEBUG("UDP", "Left");          break;
//         case 'd': turnRight();     LOG_DEBUG("UDP", "Right");         break;
//         case ' ': stopMotors();    LOG_DEBUG("UDP", "Stop");          break;
//         default: break;
//     }
// }

// void setup() {
//     delay(1000);
//     Serial.begin(115200);

//     for (int pin : MOTOR_ENABLE_PINS) {
//         pinMode(pin, OUTPUT);
//         digitalWrite(pin, HIGH);
//     }

//     L_cfg.pwm_freq_hz = 10000;
//     motor_L.setup(L_cfg); motor_L.start();

//     R_cfg.pwm_freq_hz = 10000;
//     motor_R.setup(R_cfg); motor_R.start();

//     stopMotors();

//     // ---- THIS IS WHERE IT GOES ----
//     WiFi.begin("YourHotspotName", "YourHotspotPassword");
//     while (WiFi.status() != WL_CONNECTED) {
//         delay(500);
//         Serial.print(".");
//     }
//     LOG_OK("WiFi", "Connected! IP: %s", WiFi.localIP().toString().c_str());
//     // --------------------------------

//     udp.begin(UDP_PORT);
//     LOG_OK("UDP", "Listening on port %d", UDP_PORT);

//     last_cmd_ms = millis();
// }

// void loop() {
//     int len = udp.parsePacket();
//     if (len > 0) {
//         char buf[8] = {0};
//         udp.read(buf, sizeof(buf) - 1);
//         handleCmd(buf[0]);
//     }

//     if (millis() - last_cmd_ms > CMD_TIMEOUT_MS) {
//         stopMotors();
//     }

//     delay(5);
// }