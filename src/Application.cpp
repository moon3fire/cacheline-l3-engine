#include "Application.h"

#include <cstring>
#include <iostream>
#include <memory>
#include <thread>

namespace cacheline {

Application::Application() noexcept
    : m_memoryArena(256u * 1024u * 1024u) {
}

Application::~Application() = default;

bool Application::Initialize(int argc, char* argv[]) noexcept {
    m_running.store(true, std::memory_order_relaxed);
    m_runSelfTest = false;
    m_inputPath.clear();
    m_readFromStdin = false;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--self-test") == 0 || std::strcmp(argv[i], "--test") == 0) {
            m_runSelfTest = true;
        } else if (std::strcmp(argv[i], "--input") == 0 && i + 1 < argc) {
            m_inputPath = argv[++i];
        } else if (std::strcmp(argv[i], "--stdin") == 0) {
            m_readFromStdin = true;
        }
    }

    if (!SetupMemoryArena()) {
        std::cerr << "Failed to initialize memory arena\n";
        return false;
    }

    if (!SetupOrderBook()) {
        std::cerr << "Failed to initialize order book\n";
        return false;
    }

    return true;
}

bool Application::ShouldRunSelfTest() const noexcept {
    return m_runSelfTest;
}

bool Application::RunSelfTest() noexcept {
    return tests::RunOrderBookSmokeTest();
}

void Application::Run() noexcept {
    if (!m_orderBook.has_value()) {
        return;
    }

    auto inputQueue = std::make_unique<SpscQueue<NetworkFrame, DEFAULT_RING_BUFFER_CAPACITY>>();
    auto tradeQueue = std::make_unique<SpscQueue<std::vector<Trade>, DEFAULT_RING_BUFFER_CAPACITY>>();
    std::atomic<bool> engineDone{false};

    m_orderBook->Start();

    InputProcessor<DEFAULT_RING_BUFFER_CAPACITY> inputProcessor(*inputQueue);
    OutputProcessor<DEFAULT_RING_BUFFER_CAPACITY> outputProcessor(*tradeQueue, engineDone);

    if (!m_inputPath.empty()) {
        inputProcessor.Start(m_inputPath);
    } else {
        inputProcessor.Start(std::cin);
    }

    outputProcessor.Start();

    std::thread engineThread([&]() noexcept {
        NetworkFrame frame{};
        while (true) {
            if (inputQueue->Pop(frame)) {
                const auto trades = m_orderBook->ProcessOrder(frame);
                if (!trades.empty()) {
                    while (!tradeQueue->Emplace(trades)) {
                        std::this_thread::yield();
                    }
                }
                continue;
            }

            if (!inputProcessor.IsRunning() && inputQueue->Empty()) {
                break;
            }

            std::this_thread::yield();
        }

        engineDone.store(true, std::memory_order_release);
    });

    inputProcessor.Stop();
    engineThread.join();
    outputProcessor.Stop();
    m_orderBook->Stop();

    std::cout << "cacheline-l3-engine running\n";
    std::cout << "best bid: " << m_orderBook->GetBestBid(0) << "\n";
    std::cout << "best ask: " << m_orderBook->GetBestAsk(0) << "\n";
}

void Application::Stop() noexcept {
    m_running.store(false, std::memory_order_relaxed);
}

bool Application::SetupMemoryArena() noexcept {
    return m_memoryArena.Initialize();
}

bool Application::SetupOrderBook() noexcept {
    if (m_orderBook.has_value()) {
        return true;
    }

    try {
        m_orderBook.emplace(m_memoryArena);
        return true;
    } catch (...) {
        return false;
    }
}

void Application::PinCurrentThread(uint32_t currentCoreID) noexcept {
    (void)currentCoreID;
}

} // namespace cacheline
