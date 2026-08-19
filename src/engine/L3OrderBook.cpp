#include "L3OrderBook.h"

namespace cacheline
{

    void PriceLevel::Append(Order *order) noexcept
    {
        order->next = nullptr;
        order->prev = tail;
        if (tail)
            tail->next = order;
        else
            head = order;

        tail = order;
        totalVolume += order->qty;
        ++orderCount;
    }

    void PriceLevel::Remove(Order *order) noexcept
    {
        if (order->prev)
            order->prev->next = order->next;
        else
            head = order->next;

        if (order->next)
            order->next->prev = order->prev;
        else
            tail = order->prev;

        totalVolume -= order->qty;
        --orderCount;
    }

    L3OrderBook::L3OrderBook(PmrArena &arena)
        : m_resource(arena.GetResource())
        , m_orders(m_resource)
        , m_bids(std::greater<uint32_t>(), m_resource)
        , m_asks(std::less<uint32_t>(), m_resource)
    {}

    void L3OrderBook::ProcessFrame(const NetworkFrame &frame) noexcept
    {
        switch (frame.msgType)
        {
        case MessageType::Add:
            AddOrder(frame.orderID, frame.price, frame.qty, frame.side);
            break;
        case MessageType::Cancel:
            CancelOrder(frame.orderID);
            break;
        case MessageType::Modify:
            ModifyOrder(frame.orderID, frame.qty);
            break;
        case MessageType::Execute:
            ExecuteOrder(frame.orderID, frame.qty);
            break;
        }
    }

    [[nodiscard]] uint32_t L3OrderBook::GetBestBid() const noexcept
    {
        return m_bids.empty() ? 0 : m_bids.begin()->first;
    }

    [[nodiscard]] uint32_t L3OrderBook::GetBestAsk() const noexcept
    {
        return m_asks.empty() ? 0 : m_asks.begin()->first;
    }

    Order *L3OrderBook::AllocateOrder(uint64_t orderID, uint32_t price, uint32_t qty, Side side) noexcept
    {
        Order *order = nullptr;
        if (m_freeList)
        {
            order = m_freeList;
            m_freeList = m_freeList->next;
        }
        else
        {
            void *ptr = m_resource->allocate(sizeof(Order), alignof(Order));
            order = static_cast<Order *>(ptr);
        }

        new (order) Order{orderID, price, qty, side, nullptr, nullptr};
        return order;
    }

    void L3OrderBook::FreeOrder(Order *order) noexcept
    {
        order->next = m_freeList;
        m_freeList = order;
    }

    void L3OrderBook::AddOrder(uint64_t orderID, uint32_t price, uint32_t qty, Side side) noexcept
    {
        Order *order = AllocateOrder(orderID, price, qty, side);

        auto [it, inserted] = m_orders.try_emplace(orderID, order);
        if (!inserted)
        {
            FreeOrder(order);
            return;
        }

        auto appendToTree = [&](auto& tree) {
            auto& level = tree[price];
            level.price = price;
            level.Append(order);
        };

        if (side == Side::Buy)
            appendToTree(m_bids);
        else
            appendToTree(m_asks);

    }

    void L3OrderBook::CancelOrder(uint64_t orderID) noexcept
    {
        auto it = m_orders.find(orderID);
        if (it == m_orders.end())
            return;

        Order *order = it->second;

        auto cancelFromTree = [&](auto &tree)
        {
            auto levelIt = tree.find(order->price);
            if (levelIt != tree.end())
            {
                levelIt->second.Remove(order);
                if (levelIt->second.orderCount == 0)
                    tree.erase(levelIt);
            }
        };

        if (order->side == Side::Buy)
            cancelFromTree(m_bids);
        else
            cancelFromTree(m_asks);

        m_orders.erase(it);
        FreeOrder(order);
    }

    void L3OrderBook::ModifyOrder(uint64_t orderId, uint32_t newQty) noexcept
    {
        if (newQty == 0)
        {
            CancelOrder(orderId);
            return;
        }

        auto it = m_orders.find(orderId);
        if (it == m_orders.end())
            return;

        Order *order = it->second;
        const uint32_t oldQty = order->qty;

        if (oldQty == newQty)
            return;

        auto modifyTree = [&](auto &tree)
        {
            auto levelIt = tree.find(order->price);
            if (levelIt != tree.end())
            {
                auto &level = levelIt->second;

                if (newQty > oldQty)
                {
                    level.Remove(order);
                    order->qty = newQty;
                    level.Append(order);
                }
                else
                {
                    const uint32_t diff = oldQty - newQty;
                    if (diff <= level.totalVolume)
                        level.totalVolume -= diff;
                    else
                        level.totalVolume = 0;

                    order->qty = newQty;
                }
            }
        };

        if (order->side == Side::Buy)
            modifyTree(m_bids);
        else
            modifyTree(m_asks);
    }

    void L3OrderBook::ExecuteOrder(uint64_t orderId, uint32_t qtyToExecute) noexcept
    {
        auto it = m_orders.find(orderId);
        if (it == m_orders.end())
            return;

        Order *order = it->second;

        if (qtyToExecute >= order->qty)
            CancelOrder(orderId);
        else
            ModifyOrder(orderId, order->qty - qtyToExecute);

    }

} // namespace cacheline