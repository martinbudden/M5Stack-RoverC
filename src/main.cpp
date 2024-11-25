#include "RoverC.h"

#include <AtomJoyStickReceiver.h>

#include <HardwareSerial.h>
#include <M5Unified.h>
#include <WiFi.h>

#if !defined(JOYSTICK_CHANNEL)
static constexpr uint8_t JOYSTICK_CHANNEL = 3;
#endif

static AtomJoyStickReceiver *atomJoyStickReceiver;
static RoverC * rover;

enum { SCREEN_HEIGHT_M5_STICK_C = 80, SCREEN_HEIGHT_M5_STICK_C_PLUS = 135 };
static int screenHeight;

#if defined(ATOM_JOYSTICK_MAC_ADDRESS)
static const uint8_t atomJoyStickMacAddress[ESP_NOW_ETH_ALEN] = ATOM_JOYSTICK_MAC_ADDRESS;
#else
static const uint8_t *atomJoyStickMacAddress = nullptr;
#endif

static uint8_t myMacAddress[ESP_NOW_ETH_ALEN];

static void displayMyMacAddress();
static void updateScreen(float throttle, float roll, float pitch, float yaw, float speed, float angle);
static void updateButtons();
static bool updateReceiver();


/*!
Main program setup:
1. Initialize the M5
2. Setup the screen
3. Get and display my MAC address
4. Initialize the joystick receiver
5. Initialize the Rover
*/
void setup()
{
    M5.begin();
    //Serial.begin(115200);
    Serial.begin(9600);

    M5.Power.begin();
    // with additional battery, we need to increase charge current
    M5.Power.setChargeCurrent(360);

    M5.Lcd.setRotation(1); // set to default, to find screen height
    screenHeight = M5.Lcd.height();
    M5.Lcd.setTextSize(screenHeight == SCREEN_HEIGHT_M5_STICK_C ? 1 : 2);
    M5.Lcd.setRotation(0);

    // Set WiFi to station mode and disconnect from Access Point if it was previously connected
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    // get my MAC address
    WiFi.macAddress(myMacAddress);
    Serial.printf("MAC ADDRESS: %02X:%02X:%02X:%02X:%02X:%02X\r\n", myMacAddress[0], myMacAddress[1], myMacAddress[2], myMacAddress[3], myMacAddress[4], myMacAddress[5]);

    displayMyMacAddress();

    static AtomJoyStickReceiver atomJoyStickReceiverStatic(myMacAddress);
    atomJoyStickReceiver = &atomJoyStickReceiverStatic;
    const esp_err_t err = atomJoyStickReceiver->init(JOYSTICK_CHANNEL, atomJoyStickMacAddress);// NOLINT(cppcoreguidelines-init-variables)
    Serial.printf("ESP-NOW Ready:%X\r\n", err);

    static RoverC roverStatic;
    rover = &roverStatic;

    if (M5.BtnB.wasPressed()) {
        // Holding BtnB down while switching on initiates binding.
        atomJoyStickReceiver->broadcastMyMacAddressForBinding();
    }
}

/*!
Main program loop:
1. Check if any buttons were pressed, and act accordingly
2. Check if any packets have been received and if so send the control values to the Rover and update the screen with those values
3. Implement a fail safe - if packets have not been received for a while, assume contact has been lost with the joystick and stop the Rover
*/
void loop()
{

    M5.update(); // Read the keys and update speaker
    updateButtons();

    static uint32_t failSafeCount {0};
    ++failSafeCount;
    if (updateReceiver()) {
        failSafeCount = 0;
    } else if (failSafeCount > 50) {
        // we've been around this loop 50 times without a packet, so we seemed to have lost contact with the receiver, so stop the rover.
        rover->stop();
        failSafeCount = 0;
    }

    delayMicroseconds(20);
}

/*!
Handle any button presses - BtnB initiates pairing.
*/
static void updateButtons()
{
    //M5Stick C/CPlus: BtnA, BtnB, BtnPWR
    static const int posX = (screenHeight == SCREEN_HEIGHT_M5_STICK_C) ? 70 : 120;
    static const int posY = (screenHeight == SCREEN_HEIGHT_M5_STICK_C) ? 115 : 200;

    if (M5.BtnA.wasPressed()) {
        M5.Lcd.setCursor(posX, posY);
        M5.Lcd.print('A');
    } else if (M5.BtnA.wasReleased()) {
        M5.Lcd.setCursor(posX, posY);
        M5.Lcd.printf("  ");
    }
    if (M5.BtnB.wasPressed()) {
        M5.Lcd.setCursor(posX, posY);
        M5.Lcd.print('B');
    } else if (M5.BtnB.wasReleased()) {
        // B button initiates binding
        atomJoyStickReceiver->broadcastMyMacAddressForBinding();
        M5.Lcd.setCursor(posX, posY);
        M5.Lcd.printf("  ");
    }
    if (M5.BtnPWR.wasPressed()) {
        M5.Lcd.setCursor(posX, posY);
        M5.Lcd.print('P');
    } else if (M5.BtnPWR.wasReleased()) {
        M5.Lcd.setCursor(posX, posY);
        M5.Lcd.printf("  ");
    } else if (M5.BtnPWR.wasDoubleClicked()) {
        // double click of BtnB switches off
        M5.Lcd.setCursor(posX, posY);
        M5.Lcd.print('P');
        M5.Power.powerOff();
    }
}

/*!
Utility function to display the Rover's MAC address on the screen.
*/
static void displayMyMacAddress()
{
    constexpr int xPos {5};
    constexpr int yOffset {5};
    const int yInc = screenHeight == SCREEN_HEIGHT_M5_STICK_C ? 10 : 20;

    M5.Lcd.setCursor(xPos, yOffset);
    M5.Lcd.printf("R:%02X:%02X:%02X", myMacAddress[0], myMacAddress[1], myMacAddress[2]);
    M5.Lcd.setCursor(xPos, yInc + yOffset);
    M5.Lcd.printf("  %02X:%02X:%02X", myMacAddress[3], myMacAddress[4], myMacAddress[5]);
}

/*!
Utility function to display the joystick's MAC address on the screen.
*/
static void displayJoyStickMacAddress(const uint8_t *tma)
{
    constexpr int xPos {5};
    constexpr int yOffset {5};
    const int yInc = screenHeight == SCREEN_HEIGHT_M5_STICK_C ? 10 : 20;

    M5.Lcd.setCursor(xPos, 2 * yInc + yOffset);
    M5.Lcd.printf("J:%02X:%02X:%02X", tma[0], tma[1], tma[2]);
    M5.Lcd.setCursor(xPos, 3* yInc + yOffset);
    M5.Lcd.printf("  %02X:%02X:%02X", tma[3], tma[4], tma[5]);
}

/*!
Update the screen for an M5StickC sized screen.
*/
static void updateScreen80x160(float throttle, float roll, float pitch, float yaw)
{
    constexpr int xPos {5};
    int yPos {60};
    M5.Lcd.setCursor(xPos, yPos);
    M5.Lcd.printf("T:%6.3f", throttle);
    yPos += 10;
    M5.Lcd.setCursor(xPos, yPos);
    M5.Lcd.printf("R:%6.3f", roll);
    yPos += 10;
    M5.Lcd.setCursor(xPos, yPos);
    M5.Lcd.printf("P:%6.3f", pitch);
    yPos += 10;
    M5.Lcd.setCursor(xPos, yPos);
    M5.Lcd.printf("Y:%6.3f", yaw);
    yPos += 10;
    M5.Lcd.setCursor(xPos, yPos);
    M5.Lcd.printf("M%d A%d F%d", atomJoyStickReceiver->getMode(), atomJoyStickReceiver->getAltMode(), atomJoyStickReceiver->getFlipButton());

    yPos += 25;
    M5.Lcd.setCursor(0, yPos);
    M5.Lcd.printf("S%4.0f", rover->getSpeed());
    M5.Lcd.setCursor(40, yPos);
    M5.Lcd.printf("A%4.0f", rover->getAngle());
}

/*!
Update the screen for an M5StickC PLUS sized screen.
*/
static void updateScreen135x240(float throttle, float roll, float pitch, float yaw)
{
    constexpr int xPos {5};
    int yPos {95};
    M5.Lcd.setCursor(xPos, yPos);
    M5.Lcd.printf("T:%6.3f", throttle);
    yPos += 20;
    M5.Lcd.setCursor(xPos, yPos);
    M5.Lcd.printf("R:%6.3f", roll);
    yPos += 20;
    M5.Lcd.setCursor(xPos, yPos);
    M5.Lcd.printf("P:%6.3f", pitch);
    yPos += 20;
    M5.Lcd.setCursor(xPos, yPos);
    M5.Lcd.printf("Y:%6.3f", yaw);
    yPos += 20;
    M5.Lcd.setCursor(xPos, yPos);
    M5.Lcd.printf("M%d A%d F%d", atomJoyStickReceiver->getMode(), atomJoyStickReceiver->getAltMode(), atomJoyStickReceiver->getFlipButton());

    yPos += 50;
    M5.Lcd.setCursor(0, yPos);
    M5.Lcd.printf("S%4.0f", rover->getSpeed());
    M5.Lcd.setCursor(68, yPos);
    M5.Lcd.printf("A%4.0f", rover->getAngle());
}

/*!
Update the screen displaying values received from the joystick, and the Rover's speed and angle.
*/
static void updateScreen(float throttle, float roll, float pitch, float yaw)
{
    if (screenHeight == SCREEN_HEIGHT_M5_STICK_C) {
        updateScreen80x160(throttle, roll, pitch, yaw);
    } else {
        updateScreen135x240(throttle, roll, pitch, yaw);
    }
}

/*!
If a packet has been received from the joystick then
1. Unpack it
2. Send the stick values to the rover move command
3. Update the screen with the stick values

Additionally, if the joystick MAC address has not been displayed yet, display it.

Returns true if a packet has been received.
*/
static bool updateReceiver()
{
    static uint32_t packetCount {0};
    static bool joystickAddressDisplayed {false};

    if (!joystickAddressDisplayed) {
        if (atomJoyStickReceiver->isPrimaryPeerMacAddressSet()) {
            joystickAddressDisplayed = true;
            const uint8_t * const tma = atomJoyStickReceiver->getPrimaryPeerMacAddress();// NOLINT(cppcoreguidelines-init-variables)
            displayJoyStickMacAddress(tma);
            Serial.printf("TRANSMIT MAC ADDRESS: %02X:%02X:%02X:%02X:%02X:%02X\r\n", tma[0], tma[1], tma[2], tma[3], tma[4], tma[5]);
        }
    }

    if (!atomJoyStickReceiver->isPacketEmpty()) {
        ++packetCount;
        if (atomJoyStickReceiver->unpackPacket()) {
            if (packetCount == 5) {
                // set the JoyStick bias so that the current readings are zero.
                atomJoyStickReceiver->setCurrentReadingsToBias();
            }
            const float throttle = atomJoyStickReceiver->getThrottle();
            const float roll = atomJoyStickReceiver->getRoll();
            const float pitch = atomJoyStickReceiver->getPitch();
            const float yaw = atomJoyStickReceiver->getYaw();
            const RoverC::control_mode_t controlMode = atomJoyStickReceiver->getMode() == AtomJoyStickReceiver::MODE_STABLE ? RoverC::MECANUM_MODE : RoverC::TANK_MODE;

            rover->move(throttle, roll, pitch, yaw, controlMode);

            updateScreen(throttle, roll, pitch, yaw);

            return true;
        }
        Serial.printf("updateReceiver Bad packet\r\n");
    }
    return false;
}

