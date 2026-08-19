#pragma once

#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

#include "core/SpscQueue.h"
#include "engine/L3OrderBook.h"

namespace cacheline {

template <size_t Capacity>
class OutputProcessor {
public:
    OutputProcessor(SpscQueue<std::vector<Trade>, Capacity> &queue,
                    std::atomic<bool> &engineDone) noexcept
        : m_queue(queue)
        , m_engineDone(engineDone)
    {}

    void Start() noexcept {
        m_running.store(true, std::memory_order_release);
        m_worker = std::thread([this]() noexcept {
            Run();
            m_running.store(false, std::memory_order_release);
        });
    }

    void Stop() noexcept {
        m_running.store(false, std::memory_order_release);
        if (m_worker.joinable()) {
            m_worker.join();
        }
    }

    [[nodiscard]] bool IsRunning() const noexcept {
        return m_running.load(std::memory_order_acquire);
    }

    void Run() noexcept {
        std::vector<Trade> trades;
        while (m_running.load(std::memory_order_acquire)) {
            if (m_queue.Pop(trades)) {
                for (const auto &trade : trades) {
                    std::cout << "[pipeline trade] symbol=" << trade.symbol
                              << " buyer=" << trade.buyerOrderId
                              << " seller=" << trade.sellerOrderId
                              << " qty=" << trade.qty
                              << " price=" << trade.price << "\n";
                }
                continue;
            }

            if (m_engineDone.load(std::memory_order_acquire)) {
                break;
            }

            std::this_thread::yield();
        }
    }

private:
    SpscQueue<std::vector<Trade>, Capacity> &m_queue;
    std::atomic<bool> &m_engineDone;
    std::atomic<bool> m_running{false};
    std::thread m_worker;
};

} // namespace cacheline
