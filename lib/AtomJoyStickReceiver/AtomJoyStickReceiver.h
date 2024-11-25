# pragma once

#include <ESPNOW_Transceiver.h>


/*!
Receiver compatible with the M5Stack Atom JoyStick.
*/
class AtomJoyStickReceiver {
public:
    explicit AtomJoyStickReceiver(const uint8_t* myMacAddress);
    esp_err_t init(uint8_t channel, const uint8_t* transmitMacAddress);
public:
    enum { DEFAULT_BROADCAST_COUNT = 20, DEFAULT_BROADCAST_DELAY_MS = 50 };
public:
    enum { MODE_STABLE = 0, MODE_SPORT = 1 };
    enum { ALT_MODE_AUTO = 4, ALT_MODE_MANUAL = 5};
private:
    enum { PACKET_SIZE = 25 };
    enum { THROTTLE = 0, ROLL = 1, PITCH = 2, YAW = 3, CONTROL_COUNT = 4 };
public:
    inline ESPNOW_Transceiver& getTransceiver(void) { return _transceiver; }
    inline esp_err_t sendData(uint8_t *data, uint16_t len) const {return _transceiver.sendData(data, len);}
    inline bool isPrimaryPeerMacAddressSet(void) const { return _transceiver.isPrimaryPeerMacAddressSet(); }
    inline const uint8_t *getPrimaryPeerMacAddress(void) const { return _transceiver.getPrimaryPeerMacAddress(); }
    inline bool isPacketEmpty(void) const { return _received_data.len == 0 ? true : false;  }
    inline void setPacketEmpty(void) { _received_data.len = 0; }
    inline const uint8_t *myMacAddress(void) const {return _transceiver.myMacAddress();}
    esp_err_t broadcastMyMacAddressForBinding(int broadcastCount=DEFAULT_BROADCAST_COUNT, int broadcastDelayMs=DEFAULT_BROADCAST_DELAY_MS) const;
public:
    enum checkPacket_t { CHECK_PACKET, DONT_CHECK_PACKET };
    bool unpackPacket(checkPacket_t checkPacket);
    inline bool unpackPacket(void) { return unpackPacket(CHECK_PACKET); }
    void resetControls(void);
    void setDeadZones(float deadZone);
    void setCurrentReadingsToBias(void);
    inline bool isBiasSet(void) const { return _biasIsSet; }
    inline float getThrottleRaw(void) const { return _controls[THROTTLE].raw; }
    inline float getRollRaw(void) const { return _controls[ROLL].raw; }
    inline float getPitchRaw(void) const { return _controls[PITCH].raw; }
    inline float getYawRaw(void) const { return _controls[YAW].raw; }
    float getThrottle (void) const;
    float getRoll (void) const;
    float getPitch(void) const;
    float getYaw(void) const;
    inline uint8_t getMode(void) const { return _mode; }
    inline uint8_t getAltMode(void) const { return _altMode; }
    inline uint8_t getArmButton(void) const { return _armButton; }
    inline uint8_t getFlipButton(void) const { return _flipButton; }
    inline uint8_t getProactiveFlag(void) const { return _proactiveFlag; }
private:
    enum { MAX_BIAS_COUNT = 5 };
    struct Control {
        float raw {0.0};
        float bias {0.0};
        float deadZone {0.01};
    };
    static float normalizedControl(const Control& control, bool raw);
private:
    ESPNOW_Transceiver _transceiver;
    ESPNOW_Transceiver::received_data_t _received_data;
    uint8_t _packet[PACKET_SIZE];
    uint8_t _filler[28 - PACKET_SIZE];
    Control _controls[CONTROL_COUNT];
    int _biasIsSet {false}; //NOTE: if `bool` type is used here then `getMode()` sometimes returns incorrect value
    int _biasCount {0};
    uint8_t _mode {0};
    uint8_t _altMode {0};
    uint8_t _armButton {0};
    uint8_t _flipButton {0};
    uint8_t _proactiveFlag {0};
};

