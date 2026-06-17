#include "ble-srvice.h"

static constexpr uint16_t FTMS_SERVICE_UUID = 0x1826;
static constexpr uint16_t ROWER_DATA_CHARACTERISTIC_UUID = 0x2AD2;
static constexpr size_t ROW_DATA_MAX_SIZE = 16;

BleFtmsService::BleFtmsService(const char* deviceName)
    : _server(nullptr),
      _service(nullptr),
      _rowerDataChar(nullptr),
      _deviceName(deviceName),
      _connected(false),
      _hasAverageStrokeRate(false),
      _hasTotalDistance(false),
      _hasInstantaneousPower(false),
      _averageStrokeRate(0),
      _totalDistanceMeters(0),
      _instantaneousPowerWatts(0),
      _serverCallbacks(*this) {
}

BleFtmsService::ServerCallbacks::ServerCallbacks(BleFtmsService& parent)
    : _parent(parent) {
}

void BleFtmsService::ServerCallbacks::onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
    _parent._connected = true;
}

void BleFtmsService::ServerCallbacks::onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
    _parent._connected = false;
    NimBLEDevice::startAdvertising();
}

void BleFtmsService::begin() {
    NimBLEDevice::init(_deviceName);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    initServer();
    initService();

    NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
    advertising->addServiceUUID(NimBLEUUID(FTMS_SERVICE_UUID));
    advertising->start();
}

void BleFtmsService::initServer() {
    _server = NimBLEDevice::createServer();
    _server->setCallbacks(&_serverCallbacks);
}

void BleFtmsService::initService() {
    _service = _server->createService(NimBLEUUID(FTMS_SERVICE_UUID));
    _rowerDataChar = _service->createCharacteristic(
        NimBLEUUID(ROWER_DATA_CHARACTERISTIC_UUID),
        NIMBLE_PROPERTY::NOTIFY
    );
}

bool BleFtmsService::isConnected() const {
    return _connected;
}

void BleFtmsService::setAverageStrokeRate(uint8_t strokeRate) {
    _averageStrokeRate = strokeRate;
    _hasAverageStrokeRate = true;
}

void BleFtmsService::setTotalDistance(uint32_t totalDistanceMeters) {
    _totalDistanceMeters = totalDistanceMeters;
    _hasTotalDistance = true;
}

void BleFtmsService::setInstantaneousPower(uint16_t powerWatts) {
    _instantaneousPowerWatts = powerWatts;
    _hasInstantaneousPower = true;
}

void BleFtmsService::notifyRowerData() {
    if (!_rowerDataChar)
        return;

    uint8_t buffer[ROW_DATA_MAX_SIZE] = {0};
    size_t length = buildRowerData(buffer, sizeof(buffer));
    if (length == 0)
        return;

    _rowerDataChar->setValue(buffer, length);
    _rowerDataChar->notify();
}

uint16_t BleFtmsService::buildFlags() const {
    uint16_t flags = 0;
    if (_hasAverageStrokeRate) {
        flags |= 0x0002; // Average Stroke Rate present
    }
    if (_hasTotalDistance) {
        flags |= 0x0004; // Total Distance present
    }
    if (_hasInstantaneousPower) {
        flags |= 0x0020; // Instantaneous Power present
    }
    return flags;
}

size_t BleFtmsService::buildRowerData(uint8_t* buffer, size_t bufferSize) const {
    if (!buffer || bufferSize < 4)
        return 0;

    uint16_t flags = buildFlags();
    buffer[0] = flags & 0xFF;
    buffer[1] = (flags >> 8) & 0xFF;
    size_t offset = 2;

    if (_hasAverageStrokeRate) {
        buffer[offset++] = _averageStrokeRate;
    }
    if (_hasTotalDistance) {
        if (offset + 3 > bufferSize)
            return 0;
        buffer[offset++] = _totalDistanceMeters & 0xFF;
        buffer[offset++] = (_totalDistanceMeters >> 8) & 0xFF;
        buffer[offset++] = (_totalDistanceMeters >> 16) & 0xFF;
    }
    if (_hasInstantaneousPower) {
        if (offset + 2 > bufferSize)
            return 0;
        buffer[offset++] = _instantaneousPowerWatts & 0xFF;
        buffer[offset++] = (_instantaneousPowerWatts >> 8) & 0xFF;
    }

    return offset;
}
