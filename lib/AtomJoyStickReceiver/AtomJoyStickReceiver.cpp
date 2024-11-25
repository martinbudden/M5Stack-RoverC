#include <AtomJoyStickReceiver.h>
#include <HardwareSerial.h>


// cppcheck-suppress uninitMemberVar
AtomJoyStickReceiver::AtomJoyStickReceiver(const uint8_t* myMacAddress) : // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
    _transceiver(myMacAddress),
    _received_data(_packet, sizeof(_packet))
    {}

esp_err_t AtomJoyStickReceiver::init(uint8_t channel, const uint8_t* transmitMacAddress)
{
    return _transceiver.init(_received_data, channel, transmitMacAddress);
}

esp_err_t AtomJoyStickReceiver::broadcastMyMacAddressForBinding(int broadcastCount, int broadcastDelayMs) const
{
    // peer command as used by the StampFlyController, see: https://github.com/m5stack/Atom-JoyStick/blob/main/examples/StampFlyController/src/main.cpp#L117
    static const uint8_t peerCommand[4] { 0xaa, 0x55, 0x16, 0x88 };
    uint8_t data[16];
    static_assert(sizeof(data) > sizeof(peerCommand) + ESP_NOW_ETH_ALEN + 2);

    data[0] = _transceiver.getBroadcastChannel();
    memcpy(&data[1], _transceiver.myMacAddress(), ESP_NOW_ETH_ALEN);
    memcpy(&data[1 + ESP_NOW_ETH_ALEN], peerCommand, sizeof(peerCommand));

    for (int ii = 0; ii < broadcastCount; ++ii) {
        if (esp_err_t err = _transceiver.broadcastData(data, sizeof(data)) != ESP_OK) {
            Serial.printf("broadcastMyMacAddressForBinding failed: %X\r\n", err);
            return err;
        }
        delay(broadcastDelayMs); // Arduino delay() function has units of milliseconds
    }
    return ESP_OK;
}

/*!
Check the packet if `checkPacket` set. If the packet is valid then unpack it into the member data and set the packet to empty.

Returns true if a valid packet received, false otherwise.
*/
bool AtomJoyStickReceiver::unpackPacket(checkPacket_t checkPacket)
{
    // see https://github.com/M5Fly-kanazawa/AtomJoy2024June/blob/main/src/main.cpp#L560 for packet format
    if (isPacketEmpty()) {
        return false;
    }

    uint8_t checksum = 0;
    for (int ii = 0; ii < PACKET_SIZE - 1; ++ii) {
        checksum += _packet[ii];
    }
    if ((checkPacket == CHECK_PACKET) && checksum != _packet[PACKET_SIZE - 1]) {
        //Serial.printf("checksum:%d, packet[24]:%d, packet[0]:%d, len:%d\r\n", checksum, _packet[24], _packet[0], receivedDataLen());
        setPacketEmpty();
        return false;
    }

    const uint8_t *macAddress = myMacAddress();
    if (checkPacket == DONT_CHECK_PACKET) {
        Serial.printf("packet: %02X:%02X:%02X\r\n", _packet[0], _packet[1], _packet[2]);
        //Serial.printf("peer:   %02X:%02X:%02X\r\n", _primaryPeerInfo.peer_addr[3], _primaryPeerInfo.peer_addr[4], _primaryPeerInfo.peer_addr[5]);
        //Serial.printf("my:     %02X:%02X:%02X\r\n", macAddress[3], macAddress[4], macAddress[5]);
    }
    if ((checkPacket == CHECK_PACKET) && (_packet[0] != macAddress[3] || _packet[1] != macAddress[4] || _packet[2] != macAddress[5])) {
        //Serial.printf("packet: %02X:%02X:%02X\r\n", _packet[0], _packet[1], _packet[2]);
        //Serial.printf("my:     %02X:%02X:%02X\r\n", macAddress[3], macAddress[4], macAddress[5]);
        setPacketEmpty();
        return false;
    }

    // cppcheck-suppress invalidPointerCast
    _controls[YAW].raw = *reinterpret_cast<float*>(&_packet[3]);
    // cppcheck-suppress invalidPointerCast
    _controls[THROTTLE].raw = *reinterpret_cast<float*>(&_packet[7]);
    // cppcheck-suppress invalidPointerCast
    _controls[ROLL].raw = *reinterpret_cast<float*>(&_packet[11]);
    // cppcheck-suppress invalidPointerCast
    _controls[PITCH].raw = -*reinterpret_cast<float*>(&_packet[15]);

    _armButton = _packet[19];
    _flipButton = _packet[20];
    _mode = _packet[21];  // _mode: stable or sport
    _altMode = _packet[22];
    _proactiveFlag = _packet[23];

    setPacketEmpty();
    return true;
}

void AtomJoyStickReceiver::setCurrentReadingsToBias(void)
{
    _biasIsSet = true;
    for (auto &control : _controls) {
        control.bias = control.raw;
    }
}

float AtomJoyStickReceiver::normalizedControl(const Control& control, bool raw)
{
    if (raw) {
        return control.raw;
    }
    const float ret = control.raw - control.bias;
    constexpr float max {1.0};
    constexpr float min {-1.0};

    if (ret < -control.deadZone) {
        return -(-control.deadZone - ret) / (control.bias -control.deadZone/2 - min);
    }
    if (ret > control.deadZone) {
        return (ret - control.deadZone) / (max - control.bias - control.deadZone/2);
    }
    return 0.0F;
}

void AtomJoyStickReceiver::resetControls(void)
{
    _biasIsSet = false;
    _biasCount = 0;
    for (auto &control : _controls) {
        control.bias = 0.0F;
        control.deadZone = 0.0F;
    }
}

void AtomJoyStickReceiver::setDeadZones(float deadZone)
{
    for (auto &control : _controls) {
        control.deadZone = deadZone;
    }
}

float AtomJoyStickReceiver::getThrottle(void) const
{
    return normalizedControl(_controls[THROTTLE], !isBiasSet());
}

float AtomJoyStickReceiver::getRoll(void) const
{
    return normalizedControl(_controls[ROLL], !isBiasSet());
}

float AtomJoyStickReceiver::getPitch(void) const
{
    return normalizedControl(_controls[PITCH], !isBiasSet());
}

float AtomJoyStickReceiver::getYaw(void) const
{
    return normalizedControl(_controls[YAW], !isBiasSet());
}

