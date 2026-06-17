#ifndef BLE_SRVICE_H
#define BLE_SRVICE_H

#include <Arduino.h>
#include <NimBLEDevice.h>

class BleFtmsService {
public:
    BleFtmsService(const char* deviceName = "WaterRower");
    void begin();
    bool isConnected() const;
    void setAverageStrokeRate(uint8_t strokeRate);
    void setTotalDistance(uint32_t totalDistanceMeters);
    void setInstantaneousPower(uint16_t powerWatts);
    void notifyRowerData();

private:
    void initServer();
    void initService();
    void updateCharacteristic();
    uint16_t buildFlags() const;
    size_t buildRowerData(uint8_t* buffer, size_t bufferSize) const;

    class ServerCallbacks : public NimBLEServerCallbacks {
    public:
        explicit ServerCallbacks(BleFtmsService& parent);
        void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override;
        void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override;
    private:
        BleFtmsService& _parent;
    };

    NimBLEServer* _server;
    NimBLEService* _service;
    NimBLECharacteristic* _rowerDataChar;
    const char* _deviceName;
    bool _connected;
    bool _hasAverageStrokeRate;
    bool _hasTotalDistance;
    bool _hasInstantaneousPower;
    uint8_t _averageStrokeRate;
    uint32_t _totalDistanceMeters;
    uint16_t _instantaneousPowerWatts;
    ServerCallbacks _serverCallbacks;
};

#endif // BLE_SRVICE_H
