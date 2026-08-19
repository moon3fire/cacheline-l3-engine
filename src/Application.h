#pragma once

#include <atomic>
#include <cstdint>
#include <optional>
#include <thread>

#include "core/PmrArena.h"
#include "core/SpscQueue.h"
#include "engine/L3OrderBook.h"
#include "io/InputProcessor.h"
#include "io/OutputProcessor.h"
#include "network/NetworkFrame.h"
#include "tests/OrderBookSmokeTest.h"

namespace cacheline {

constexpr size_t DEFAULT_RING_BUFFER_CAPACITY = 1 << 16;

struct Config {
    uint32_t networkCoreID{1};
    uint32_t engineCoreID{1};
    size_t arenaSizeMb{256};
    const char* networkInterface{"eth0"};
    uint16_t udpPort{12012};
};

class Application {
public:
    Application() noexcept;
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    [[nodiscard]] bool Initialize(int argc, char* argv[]) noexcept;
    [[nodiscard]] bool ShouldRunSelfTest() const noexcept;
    [[nodiscard]] bool RunSelfTest() noexcept;
    void Run() noexcept;
    void Stop() noexcept;

private:
    bool SetupMemoryArena() noexcept;
    bool SetupOrderBook() noexcept;
    void PinCurrentThread(uint32_t currentCoreID) noexcept;

private:
    Config m_config{};
    std::atomic<bool> m_running{false};
    bool m_runSelfTest{false};
    std::string m_inputPath;
    bool m_readFromStdin{false};

    PmrArena m_memoryArena;
    SpscQueue<NetworkFrame, DEFAULT_RING_BUFFER_CAPACITY> m_frameQueue;
    std::optional<L3OrderBook> m_orderBook;
};

} // namespace cacheline