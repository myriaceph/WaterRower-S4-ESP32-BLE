#include "waterrower.h"

const WaterRower::MemoryInfo WaterRower::MEMORY_MAP[] = {
    {"055", "total_distance_m", "double", 16, false},
    {"140", "total_strokes", "double", 16, false},
    {"088", "watts", "double", 16, false},
    {"08A", "total_kcal", "triple", 16, false},
    {"14A", "avg_distance_cmps", "double", 16, false},
    {"148", "total_speed_cmps", "double", 16, false},
    {"1E0", "display_sec_dec", "single", 10, false},
    {"1E1", "display_sec", "single", 10, false},
    {"1E2", "display_min", "single", 10, false},
    {"1E3", "display_hr", "single", 10, false},
    {"1A0", "heart_rate", "double", 16, false},
    {"1A6", "500mps", "double", 16, false},
    {"1A9", "stroke_rate", "single", 16, false},
    {"142", "avg_time_stroke_whole", "single", 16, false},
    {"143", "avg_time_stroke_pull", "single", 16, false},
    {"0A9", "tank_volume", "single", 16, true},
};

const size_t WaterRower::MEMORY_MAP_SIZE = sizeof(WaterRower::MEMORY_MAP) / sizeof(WaterRower::MemoryInfo);
const char* WaterRower::SIZE_MAP_SINGLE = "IRS";
const char* WaterRower::SIZE_MAP_DOUBLE = "IRD";
const char* WaterRower::SIZE_MAP_TRIPLE = "IRT";

WaterRower::WaterRower(HWCDC& serial)
    : _serial(serial),
      _connected(false),
      _lastRequestMillis(0),
      _requestInterval(25),
      _requestIndex(0),
      _nextCallbackId(1) {
}

void WaterRower::begin() {
    // Serial object should already be configured by the caller.
}

void WaterRower::open() {
    _connected = true;
    _requestIndex = 0;
    _lastRequestMillis = millis();
    writeCommand("USB");
}

void WaterRower::close() {
    if (!_connected)
        return;

    writeCommand("EXIT");
    _connected = false;
}

bool WaterRower::isConnected() const {
    return _connected;
}

void WaterRower::update() {
    if (!_connected)
        return;

    while (_serial.available() > 0) {
        char c = static_cast<char>(_serial.read());
        if (c == '\n' || c == '\r') {
            if (_lineBuffer.length() > 0) {
                processLine(_lineBuffer);
                _lineBuffer.clear();
            }
            continue;
        }
        _lineBuffer += c;
    }

    unsigned long now = millis();
    if (now - _lastRequestMillis >= _requestInterval) {
        _lastRequestMillis = now;
        requestNextAddress();
    }
}

void WaterRower::requestInfo() {
    writeCommand("IV?");
    requestAddress("0A9");
}

void WaterRower::resetRequest() {
    writeCommand("RESET");
}

void WaterRower::requestAddress(const String& address) {
    const MemoryInfo* info = findMemoryInfo(address);
    if (info == nullptr)
        return;

    writeCommand(formatReadRequest(*info));
}

uint32_t WaterRower::registerCallback(WaterRowerCallback callback) {
    if (!callback)
        return 0;

    uint32_t id = _nextCallbackId++;
    _callbacks.push_back({id, callback});
    return id;
}

void WaterRower::removeCallback(uint32_t callbackId) {
    for (auto it = _callbacks.begin(); it != _callbacks.end(); ++it) {
        if (it->id == callbackId) {
            _callbacks.erase(it);
            return;
        }
    }
}

void WaterRower::setRequestInterval(unsigned long intervalMs) {
    _requestInterval = intervalMs;
}

void WaterRower::writeCommand(const String& raw) {
    String command = raw;
    command.toUpperCase();
    command += "\r\n";
    _serial.print(command);
    _serial.flush();
}

void WaterRower::processLine(const String& line) {
    String trimmed = line;
    trimmed.trim();
    if (trimmed.length() == 0)
        return;
    handleResponse(trimmed);
}

void WaterRower::handleResponse(const String& cmd) {
    WaterRowerEvent event;
    event.raw = cmd;

    if (cmd == "OK") {
        return;
    }
    if (cmd == "ERROR") {
        event.type = "error";
        notifyCallbacks(event);
        return;
    }
    if (cmd == "SS") {
        event.type = "stroke_start";
        notifyCallbacks(event);
        return;
    }
    if (cmd == "SE") {
        event.type = "stroke_end";
        notifyCallbacks(event);
        return;
    }
    if (cmd.startsWith("PING")) {
        event.type = "ping";
        notifyCallbacks(event);
        return;
    }
    if (cmd.startsWith("IV")) {
        event.type = "model";
        notifyCallbacks(event);
        return;
    }
    if (cmd.startsWith("P") && cmd.length() >= 2) {
        event.type = "pulse";
        event.valueString = cmd.substring(1);
        event.hasValue = true;
        event.value = event.valueString.toInt();
        notifyCallbacks(event);
        return;
    }
    if (cmd.startsWith("ID") && cmd.length() > 5) {
        char sizeChar = cmd.charAt(2);
        String address = cmd.substring(3, 6);
        const MemoryInfo* info = findMemoryInfo(address);
        if (info != nullptr) {
            String valueString;
            if (sizeChar == 'S') {
                valueString = cmd.substring(6, 8);
            } else if (sizeChar == 'D') {
                valueString = cmd.substring(6, 10);
            } else if (sizeChar == 'T') {
                valueString = cmd.substring(6, 12);
            }
            event.type = String(info->type);
            event.valueString = valueString;
            event.value = parseValue(valueString, info->base);
            event.hasValue = true;
            notifyCallbacks(event);
            return;
        }
    }

    event.type = "unknown";
    notifyCallbacks(event);
}

void WaterRower::notifyCallbacks(const WaterRowerEvent& event) {
    for (const auto& entry : _callbacks) {
        if (entry.callback) {
            entry.callback(event);
        }
    }
}

const WaterRower::MemoryInfo* WaterRower::findMemoryInfo(const String& address) const {
    for (size_t i = 0; i < MEMORY_MAP_SIZE; ++i) {
        if (address.equalsIgnoreCase(MEMORY_MAP[i].address)) {
            return &MEMORY_MAP[i];
        }
    }
    return nullptr;
}

String WaterRower::formatReadRequest(const MemoryInfo& info) const {
    if (strcmp(info.size, "single") == 0) {
        return String(SIZE_MAP_SINGLE) + info.address;
    }
    if (strcmp(info.size, "double") == 0) {
        return String(SIZE_MAP_DOUBLE) + info.address;
    }
    if (strcmp(info.size, "triple") == 0) {
        return String(SIZE_MAP_TRIPLE) + info.address;
    }
    return String("IRS") + info.address;
}

void WaterRower::requestNextAddress() {
    size_t startIndex = _requestIndex;
    for (size_t count = 0; count < MEMORY_MAP_SIZE; ++count) {
        size_t index = (_requestIndex + count) % MEMORY_MAP_SIZE;
        const MemoryInfo& info = MEMORY_MAP[index];
        if (!info.notInLoop) {
            _requestIndex = (index + 1) % MEMORY_MAP_SIZE;
            writeCommand(formatReadRequest(info));
            return;
        }
    }
    _requestIndex = (startIndex + 1) % MEMORY_MAP_SIZE;
}

long WaterRower::parseValue(const String& valueString, uint8_t base) const {
    if (valueString.length() == 0)
        return 0;
    if (base == 10) {
        return valueString.toInt();
    }
    char buffer[16] = {0};
    valueString.toCharArray(buffer, sizeof(buffer));
    return strtol(buffer, nullptr, base);
}
