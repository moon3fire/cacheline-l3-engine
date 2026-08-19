#include "Application.h"

#include <cstring>
#include <iostream>
#include <thread>

namespace cacheline {

Application::Application() noexcept
    : m_memoryArena(256u * 1024u * 1024u) {
}

Application::~Application() = default;

bool Application::Initialize(int argc, char* argv[]) noexcept {
    m_running.store(true, std::memory_order_relaxed);
    m_runSelfTest = false;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--self-test") == 0 || std::strcmp(argv[i], "--test") == 0) {
            m_runSelfTest = true;
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

    std::cout << "cacheline-l3-engine running\n";
    std::cout << "best bid: " << m_orderBook->GetBestBid() << "\n";
    std::cout << "best ask: " << m_orderBook->GetBestAsk() << "\n";
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
