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
    {}

    L3OrderBook::BookState &L3OrderBook::GetOrCreateBook(uint64_t symbol) noexcept
    {
        auto it = m_books.find(symbol);
        if (it != m_books.end())
            return it->second;

        auto [newIt, inserted] = m_books.emplace(symbol, BookState(m_resource));
        return newIt->second;
    }

    const L3OrderBook::BookState *L3OrderBook::FindBook(uint64_t symbol) const noexcept
    {
        auto it = m_books.find(symbol);
        return it == m_books.end() ? nullptr : &it->second;
    }

    void L3OrderBook::Start() noexcept
    {
        m_running.store(true, std::memory_order_release);
    }

    void L3OrderBook::Stop() noexcept
    {
        m_running.store(false, std::memory_order_release);
    }

    bool L3OrderBook::IsRunning() const noexcept
    {
        return m_running.load(std::memory_order_acquire);
    }

    std::vector<Trade> L3OrderBook::ProcessOrder(const NetworkFrame &frame) noexcept
    {
        std::vector<Trade> trades;
        auto &book = GetOrCreateBook(frame.symbol);

        if (frame.msgType != MessageType::Add)
        {
            ProcessFrame(frame);
            return trades;
        }

        uint32_t remainingQty = frame.qty;
        const bool isBuy = frame.side == Side::Buy;

        auto appendRestingOrder = [&](auto &incomingTree) {
            if (remainingQty == 0)
                return;

            Order *newOrder = AllocateOrder(book, frame.orderID, frame.price, remainingQty, frame.side);
            auto &level = incomingTree[frame.price];
            level.price = frame.price;
            level.Append(newOrder);
            book.m_orders[frame.orderID] = newOrder;
        };

        auto matchAgainst = [&](auto &restingTree, auto &incomingTree, auto &&crosses) {
            while (remainingQty > 0 && !restingTree.empty())
            {
                auto &bestLevel = restingTree.begin()->second;
                Order *restingOrder = bestLevel.head;
                if (!restingOrder)
                {
                    restingTree.erase(restingTree.begin());
                    continue;
                }

                const uint32_t bestPrice = bestLevel.price;
                if (!crosses(frame.price, bestPrice))
                    break;

                const uint32_t tradeQty = std::min(remainingQty, restingOrder->qty);
                const uint32_t tradePrice = bestPrice;

                Trade trade{
                    frame.symbol,
                    isBuy ? frame.orderID : restingOrder->orderID,
                    isBuy ? restingOrder->orderID : frame.orderID,
                    tradePrice,
                    tradeQty
                };
                trades.push_back(trade);

                remainingQty -= tradeQty;
                restingOrder->qty -= tradeQty;

                if (restingOrder->qty == 0)
                {
                    bestLevel.Remove(restingOrder);
                    book.m_orders.erase(restingOrder->orderID);
                    if (bestLevel.orderCount == 0)
                        restingTree.erase(restingTree.begin());
                }
            }

            appendRestingOrder(incomingTree);
        };

        if (isBuy)
            matchAgainst(book.m_asks, book.m_bids, [](uint32_t incomingPrice, uint32_t restingPrice) {
                return incomingPrice >= restingPrice;
            });
        else
            matchAgainst(book.m_bids, book.m_asks, [](uint32_t incomingPrice, uint32_t restingPrice) {
                return incomingPrice <= restingPrice;
            });

        return trades;
    }

    void L3OrderBook::ProcessFrame(const NetworkFrame &frame) noexcept
    {
        auto &book = GetOrCreateBook(frame.symbol);

        switch (frame.msgType)
        {
        case MessageType::Add:
            AddOrderToBook(book, frame.orderID, frame.price, frame.qty, frame.side);
            break;
        case MessageType::Cancel:
            CancelOrderFromBook(book, frame.orderID);
            break;
        case MessageType::Modify:
            UpdateBookOrderQty(book, frame.orderID, frame.qty);
            break;
        case MessageType::Execute:
            ReduceBookOrderQty(book, frame.orderID, frame.qty);
            break;
        }
    }

    [[nodiscard]] uint32_t L3OrderBook::GetBestBid(uint64_t symbol) const noexcept
    {
        const auto *book = FindBook(symbol);
        if (!book)
            return 0;
        return book->m_bids.empty() ? 0 : book->m_bids.begin()->first;
    }

    [[nodiscard]] uint32_t L3OrderBook::GetBestAsk(uint64_t symbol) const noexcept
    {
        const auto *book = FindBook(symbol);
        if (!book)
            return 0;
        return book->m_asks.empty() ? 0 : book->m_asks.begin()->first;
    }

    Order *L3OrderBook::AllocateOrder(BookState &book, uint64_t orderID, uint32_t price, uint32_t qty, Side side) noexcept
    {
        Order *order = nullptr;
        if (book.m_freeList)
        {
            order = book.m_freeList;
            book.m_freeList = book.m_freeList->next;
        }
        else
        {
            void *ptr = m_resource->allocate(sizeof(Order), alignof(Order));
            order = static_cast<Order *>(ptr);
        }

        new (order) Order{orderID, price, qty, side, nullptr, nullptr};
        return order;
    }

    void L3OrderBook::FreeOrder(BookState &book, Order *order) noexcept
    {
        order->next = book.m_freeList;
        book.m_freeList = order;
    }

    void L3OrderBook::AddOrderToBook(BookState &book, uint64_t orderID, uint32_t price, uint32_t qty, Side side) noexcept
    {
        Order *order = AllocateOrder(book, orderID, price, qty, side);

        auto [it, inserted] = book.m_orders.try_emplace(orderID, order);
        if (!inserted)
        {
            FreeOrder(book, order);
            return;
        }

        auto appendToTree = [&](auto& tree) {
            auto& level = tree[price];
            level.price = price;
            level.Append(order);
        };

        if (side == Side::Buy)
            appendToTree(book.m_bids);
        else
            appendToTree(book.m_asks);
    }

    void L3OrderBook::CancelOrderFromBook(BookState &book, uint64_t orderID) noexcept
    {
        auto it = book.m_orders.find(orderID);
        if (it == book.m_orders.end())
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
            cancelFromTree(book.m_bids);
        else
            cancelFromTree(book.m_asks);

        book.m_orders.erase(it);
        FreeOrder(book, order);
    }

    void L3OrderBook::UpdateBookOrderQty(BookState &book, uint64_t orderID, uint32_t newQty) noexcept
    {
        if (newQty == 0)
        {
            CancelOrderFromBook(book, orderID);
            return;
        }

        auto it = book.m_orders.find(orderID);
        if (it == book.m_orders.end())
            return;

        Order *order = it->second;
        const uint32_t oldQty = order->qty;
        if (oldQty == newQty)
            return;

        if (order->side == Side::Buy)
        {
            auto levelIt = book.m_bids.find(order->price);
            if (levelIt == book.m_bids.end())
                return;

            auto &level = levelIt->second;
            const uint32_t delta = (newQty > oldQty) ? (newQty - oldQty) : (oldQty - newQty);
            if (newQty > oldQty)
                level.totalVolume += delta;
            else
                level.totalVolume = (level.totalVolume > delta) ? (level.totalVolume - delta) : 0;

            order->qty = newQty;
            return;
        }

        auto askLevelIt = book.m_asks.find(order->price);
        if (askLevelIt == book.m_asks.end())
            return;

        auto &askLevel = askLevelIt->second;
        const uint32_t delta = (newQty > oldQty) ? (newQty - oldQty) : (oldQty - newQty);
        if (newQty > oldQty)
            askLevel.totalVolume += delta;
        else
            askLevel.totalVolume = (askLevel.totalVolume > delta) ? (askLevel.totalVolume - delta) : 0;

        order->qty = newQty;
    }

    void L3OrderBook::ReduceBookOrderQty(BookState &book, uint64_t orderID, uint32_t executedQty) noexcept
    {
        auto it = book.m_orders.find(orderID);
        if (it == book.m_orders.end())
            return;

        Order *order = it->second;
        if (executedQty >= order->qty)
        {
            CancelOrderFromBook(book, orderID);
            return;
        }

        UpdateBookOrderQty(book, orderID, order->qty - executedQty);
    }

} // namespace cacheline