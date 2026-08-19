#pragma once

#include <atomic>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "core/SpscQueue.h"
#include "network/NetworkFrame.h"

namespace cacheline {

template <size_t Capacity>
class InputProcessor {
public:
    explicit InputProcessor(SpscQueue<NetworkFrame, Capacity> &queue) noexcept
        : m_queue(queue)
    {}

    void Start(std::istream &input) noexcept {
        m_running.store(true, std::memory_order_release);
        m_worker = std::thread([this, &input]() noexcept {
            RunFromStream(input);
            m_running.store(false, std::memory_order_release);
        });
    }

    void Start(const std::filesystem::path &path) noexcept {
        if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
            m_running.store(false, std::memory_order_release);
            return;
        }

        std::ifstream input(path);
        if (!input.is_open()) {
            m_running.store(false, std::memory_order_release);
            return;
        }

        Start(input);
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

    void Run(const std::vector<NetworkFrame> &frames) noexcept {
        for (const auto &frame : frames) {
            Enqueue(frame);
        }
    }

    bool RunFromFile(const std::string &path) noexcept {
        const std::filesystem::path filePath(path);
        if (!std::filesystem::exists(filePath) || !std::filesystem::is_regular_file(filePath)) {
            return false;
        }

        std::ifstream input(filePath);
        if (!input.is_open()) {
            return false;
        }

        return RunFromStream(input);
    }

    bool RunFromStream(std::istream &input) noexcept {
        std::string line;
        while (std::getline(input, line)) {
            auto trimmed = Trim(line);
            if (trimmed.empty() || trimmed[0] == '#') {
                continue;
            }

            const auto frame = ParseLine(trimmed);
            if (frame.has_value()) {
                Enqueue(*frame);
            }
        }

        return true;
    }

    static std::optional<NetworkFrame> ParseLine(const std::string &line) noexcept {
        std::istringstream iss(line);
        std::string symbolStr;
        std::string sideStr;
        std::string orderIdStr;
        std::string priceStr;
        std::string qtyStr;
        std::string typeStr;

        if (!(iss >> symbolStr >> sideStr >> orderIdStr >> priceStr >> qtyStr >> typeStr)) {
            return std::nullopt;
        }

        NetworkFrame frame{};
        try {
            frame.symbol = std::stoull(symbolStr);
            frame.orderID = std::stoull(orderIdStr);
            frame.price = static_cast<uint32_t>(std::stoul(priceStr));
            frame.qty = static_cast<uint32_t>(std::stoul(qtyStr));
        } catch (...) {
            return std::nullopt;
        }

        auto side = ParseSide(sideStr);
        auto type = ParseMessageType(typeStr);
        if (!side.has_value() || !type.has_value()) {
            return std::nullopt;
        }

        frame.side = *side;
        frame.msgType = *type;
        return frame;
    }

private:
    void Enqueue(const NetworkFrame &frame) noexcept {
        while (!m_queue.Emplace(frame)) {
            std::this_thread::yield();
        }
    }

    static std::string Trim(const std::string &value) noexcept {
        const auto begin = value.find_first_not_of(" \t\r\n");
        if (begin == std::string::npos) {
            return {};
        }

        const auto end = value.find_last_not_of(" \t\r\n");
        return value.substr(begin, end - begin + 1);
    }

    static std::optional<Side> ParseSide(const std::string &value) noexcept {
        if (value == "BUY" || value == "Buy" || value == "buy") {
            return Side::Buy;
        }
        if (value == "SELL" || value == "Sell" || value == "sell") {
            return Side::Sell;
        }
        return std::nullopt;
    }

    static std::optional<MessageType> ParseMessageType(const std::string &value) noexcept {
        if (value == "ADD" || value == "Add" || value == "add") {
            return MessageType::Add;
        }
        if (value == "CANCEL" || value == "Cancel" || value == "cancel") {
            return MessageType::Cancel;
        }
        if (value == "MODIFY" || value == "Modify" || value == "modify") {
            return MessageType::Modify;
        }
        if (value == "EXECUTE" || value == "Execute" || value == "execute") {
            return MessageType::Execute;
        }
        return std::nullopt;
    }

private:
    SpscQueue<NetworkFrame, Capacity> &m_queue;
    std::atomic<bool> m_running{false};
    std::thread m_worker;
};

} // namespace cacheline
