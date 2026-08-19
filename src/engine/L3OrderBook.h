#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <memory_resource>
#include <unordered_map>
#include <vector>

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

    struct Trade
    {
        uint64_t symbol{0};
        uint64_t buyerOrderId{0};
        uint64_t sellerOrderId{0};
        uint32_t price{0};
        uint32_t qty{0};
    };

    class L3OrderBook
    {
    public:
        struct BookState {
            BookState() = default;
            explicit BookState(std::pmr::memory_resource *resource)
                : m_orders(resource)
                , m_bids(std::greater<uint32_t>(), resource)
                , m_asks(std::less<uint32_t>(), resource)
            {}

            std::pmr::unordered_map<uint64_t, Order *> m_orders;
            std::pmr::map<uint32_t, PriceLevel, std::greater<uint32_t>> m_bids;
            std::pmr::map<uint32_t, PriceLevel, std::less<uint32_t>> m_asks;
            Order* m_freeList { nullptr };
        };

    public:
        explicit L3OrderBook(PmrArena &arena);
        ~L3OrderBook() = default;
        
        L3OrderBook(const L3OrderBook&) = delete;
        L3OrderBook& operator=(const L3OrderBook&) = delete;
        
        L3OrderBook(L3OrderBook&&) noexcept = default; // moving can be allowed
        L3OrderBook& operator=(L3OrderBook&&) noexcept = default;
        
    public:
        void Start() noexcept;
        void Stop() noexcept;
        [[nodiscard]] bool IsRunning() const noexcept;

        std::vector<Trade> ProcessOrder(const NetworkFrame &frame) noexcept;
        void ProcessFrame(const NetworkFrame &frame) noexcept;

        [[nodiscard]] uint32_t GetBestBid(uint64_t symbol) const noexcept;
        [[nodiscard]] uint32_t GetBestAsk(uint64_t symbol) const noexcept;

    private:
        BookState &GetOrCreateBook(uint64_t symbol) noexcept;
        const BookState *FindBook(uint64_t symbol) const noexcept;

        Order* AllocateOrder(BookState &book, uint64_t orderID, uint32_t price, uint32_t qty, Side side) noexcept;
        void FreeOrder(BookState &book, Order* order) noexcept;
        void AddOrderToBook(BookState &book, uint64_t orderID, uint32_t price, uint32_t qty, Side side) noexcept;
        void CancelOrderFromBook(BookState &book, uint64_t orderID) noexcept;
        void UpdateBookOrderQty(BookState &book, uint64_t orderID, uint32_t newQty) noexcept;
        void ReduceBookOrderQty(BookState &book, uint64_t orderID, uint32_t executedQty) noexcept;
    private:
        std::pmr::memory_resource *m_resource { nullptr };
        std::unordered_map<uint64_t, BookState> m_books;
        std::atomic<bool> m_running{false};
    };

} // namespace cacheline