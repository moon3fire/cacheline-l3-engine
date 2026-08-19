#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory_resource>
#include <unordered_map>

#include "core/SpscQueue.h" // TODO:
#include "core/PmrArena.h"
#include "network/NetworkFrame.h"

namespace cacheline
{

    struct Order
    {
        uint64_t orderID{0};
        uint32_t price{0};
        uint32_t qty{0};
        Side side{Side::Buy};
        Order *prev{nullptr};
        Order *next{nullptr};
    };

    struct PriceLevel
    {
        uint32_t price{0};
        uint64_t totalVolume{0};
        uint32_t orderCount{0};
        Order *head{nullptr};
        Order *tail{nullptr};

        void Append(Order *order) noexcept;
        void Remove(Order *order) noexcept;
    };

    class L3OrderBook
    {
    public:
        explicit L3OrderBook(PmrArena &arena);
        ~L3OrderBook() = default;
        
        L3OrderBook(const L3OrderBook&) = delete;
        L3OrderBook& operator=(const L3OrderBook&) = delete;
        
        L3OrderBook(L3OrderBook&&) noexcept = default; // moving can be allowed
        L3OrderBook& operator=(L3OrderBook&&) noexcept = default;
        
    public:
        void ProcessFrame(const NetworkFrame &frame) noexcept;

        [[nodiscard]] uint32_t GetBestBid() const noexcept;
        [[nodiscard]] uint32_t GetBestAsk() const noexcept;

    private:
        Order* AllocateOrder(uint64_t orderID, uint32_t price, uint32_t qty, Side side) noexcept;
        void FreeOrder(Order* order) noexcept;
        void AddOrder(uint64_t orderID, uint32_t price, uint32_t qty, Side side) noexcept;
        void CancelOrder(uint64_t orderID) noexcept;
        void ModifyOrder(uint64_t orderID, uint32_t newQty) noexcept;
        void ExecuteOrder(uint64_t orderID, uint32_t executedQty) noexcept;
    private:
        std::pmr::memory_resource *m_resource { nullptr };

        std::pmr::unordered_map<uint64_t, Order *> m_orders;

        std::pmr::map<uint32_t, PriceLevel, std::greater<uint32_t>> m_bids;
        std::pmr::map<uint32_t, PriceLevel, std::less<uint32_t>> m_asks;

        Order* m_freeList { nullptr };
    };

} // namespace cacheline