# pragma once

#include <esp_now.h>


class ESPNOW_Transceiver {
public:
    enum { BROADCAST_PEER=0, PRIMARY_PEER=1, SECONDARY_PEER=2, PEER_2=2, PEER_3=4, MAX_PEER_COUNT=4 };
    struct received_data_t {
        inline received_data_t(uint8_t *aBufferPtr, int aBufferSize)
            : bufferPtr(aBufferPtr), bufferSize(aBufferSize), len(0) {}
        uint8_t *bufferPtr;
        int bufferSize;
        int len;
    };
    struct peer_data_t {
        esp_now_peer_info_t peer_info { .peer_addr{0,0,0,0,0,0}, .lmk{0}, .channel=0, .ifidx=WIFI_IF_STA, .encrypt=false,.priv=nullptr};
        received_data_t *receivedDataPtr {nullptr};
    };
public:
    explicit ESPNOW_Transceiver(const uint8_t* myMacAddress);
    // !!NOTE: all references passed to init() and addSecondaryPeer() must be static or allocated, ie they must not be local variables on the stack
    esp_err_t init(received_data_t& received_data, uint8_t channel, const uint8_t* transmitMacAddress);
    esp_err_t addSecondaryPeer(received_data_t& received_data, const uint8_t* macAddress);
    inline const uint8_t *myMacAddress(void) const { return _myMacAddress; }
    // sends data to the primary peer,
    esp_err_t sendData(uint8_t *data, int len) const;
    // sends data to the secondary peer,
    esp_err_t sendDataSecondary(uint8_t *data, int len) const;
    bool isPrimaryPeerMacAddressSet(void) const;
    const uint8_t *getPrimaryPeerMacAddress(void) const { return _peerData[PRIMARY_PEER].peer_info.peer_addr; }
    inline uint8_t getBroadcastChannel(void) const { return _peerData[BROADCAST_PEER].peer_info.channel; }
    esp_err_t broadcastData(const uint8_t *data, int len) const { return esp_now_send(_peerData[BROADCAST_PEER].peer_info.peer_addr, data, len); }
    inline uint32_t getReceivedPacketCount(void) const { return _receivedPacketCount; }
    inline uint32_t getTickCountDelta(void) const { return _tickCountDelta; }
private:
    esp_err_t init(uint8_t channel);
    esp_err_t addBroadcastPeer(int channel);
    // when data is received the copy function is called to copy the received data into the client's buffer
    bool copyReceivedDataToBuffer(const uint8_t *macAddress, const uint8_t *data, int len);
    bool macAddressAlreadyAdded(const uint8_t *macAddress) const;
    esp_err_t setPrimaryPeerMacAddress(const uint8_t* macAddress);
    inline void setSendStatus(esp_now_send_status_t status) { _sendStatus = status; }
private:
    friend void onDataSent(const uint8_t *macAddress, esp_now_send_status_t status);
    friend void onDataReceived(const uint8_t *macAddress, const uint8_t *data, int len);
private:
    static const uint8_t broadcastMacAddress[ESP_NOW_ETH_ALEN];
    // tick counts are used for instrumentation
    uint32_t _tickCountPrevious {0};
    uint32_t _tickCountDelta {0};
    uint32_t _receivedPacketCount {0}; //!< used to check for dropped packets
    // by default the transceiver has two peers, the broadcast peer and the primary peer
    int _isPrimaryPeerMacAddressSet {false};
    int _peerCount {0};
    peer_data_t _peerData[MAX_PEER_COUNT];
    esp_now_send_status_t _sendStatus {ESP_NOW_SEND_SUCCESS};
    uint8_t _myMacAddress[ESP_NOW_ETH_ALEN + 2]  {0, 0, 0, 0, 0, 0, 0, 0};
};

