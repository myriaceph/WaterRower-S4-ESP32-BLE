#ifndef WATERROWER_H
#define WATERROWER_H

#include <Arduino.h>
#include <HWCDC.h>
#include <functional>
#include <vector>

struct WaterRowerEvent {
    String type;
    String raw;
    long value = 0;
    String valueString;
    bool hasValue = false;
};

using WaterRowerCallback = std::function<void(const WaterRowerEvent&)>;

class WaterRower {
public:
    explicit WaterRower(HWCDC& serial);
    void begin();
    void open();
    void close();
    bool isConnected() const;
    void update();
    void requestInfo();
    void resetRequest();
    void requestAddress(const String& address);
    uint32_t registerCallback(WaterRowerCallback callback);
    void removeCallback(uint32_t callbackId);
    void setRequestInterval(unsigned long intervalMs);

private:
    struct MemoryInfo {
        const char* address;
        const char* type;
        const char* size;
        uint8_t base;
        bool notInLoop;
    };

    struct CallbackEntry {
        uint32_t id;
        WaterRowerCallback callback;
    };

    void writeCommand(const String& raw);
    void processLine(const String& line);
    void handleResponse(const String& cmd);
    void notifyCallbacks(const WaterRowerEvent& event);
    const MemoryInfo* findMemoryInfo(const String& address) const;
    String formatReadRequest(const MemoryInfo& info) const;
    void requestNextAddress();
    long parseValue(const String& valueString, uint8_t base) const;

    HWCDC& _serial;
    bool _connected;
    unsigned long _lastRequestMillis;
    unsigned long _requestInterval;
    size_t _requestIndex;
    String _lineBuffer;
    uint32_t _nextCallbackId;
    std::vector<CallbackEntry> _callbacks;

    static const MemoryInfo MEMORY_MAP[];
    static const size_t MEMORY_MAP_SIZE;
    static const char* SIZE_MAP_SINGLE;
    static const char* SIZE_MAP_DOUBLE;
    static const char* SIZE_MAP_TRIPLE;
};

#endif // WATERROWER_H
