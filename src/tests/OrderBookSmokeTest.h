#pragma once

#include "engine/L3OrderBook.h"
#include "network/NetworkFrame.h"

#include <iostream>

namespace cacheline::tests {

[[nodiscard]] inline bool RunOrderBookSmokeTest() noexcept {
    PmrArena arena(256u * 1024u * 1024u);
    if (!arena.Initialize()) {
        std::cerr << "Smoke test failed: arena init\n";
        return false;
    }

    L3OrderBook book(arena);

    auto printState = [&]() {
        std::cout << "\nState: best bid=" << book.GetBestBid()
                  << ", best ask=" << book.GetBestAsk() << '\n';
    };

    std::cout << "[orderbook smoke test] start\n";

    NetworkFrame addBid{};
    addBid.msgType = MessageType::Add;
    addBid.orderID = 1;
    addBid.price = 1000;
    addBid.qty = 50;
    addBid.side = Side::Buy;
    book.ProcessFrame(addBid);
    std::cout << "ADD BID: order=1 qty=50 price=1000\n";
    printState();

    NetworkFrame addAsk{};
    addAsk.msgType = MessageType::Add;
    addAsk.orderID = 2;
    addAsk.price = 1010;
    addAsk.qty = 40;
    addAsk.side = Side::Sell;
    book.ProcessFrame(addAsk);
    std::cout << "ADD ASK: order=2 qty=40 price=1010\n";
    printState();

    NetworkFrame modifyBid{};
    modifyBid.msgType = MessageType::Modify;
    modifyBid.orderID = 1;
    modifyBid.qty = 75;
    modifyBid.side = Side::Buy;
    book.ProcessFrame(modifyBid);
    std::cout << "MODIFY BID: order=1 new qty=75\n";
    printState();

    NetworkFrame cancelAsk{};
    cancelAsk.msgType = MessageType::Cancel;
    cancelAsk.orderID = 2;
    book.ProcessFrame(cancelAsk);
    std::cout << "CANCEL ASK: order=2\n";
    printState();

    const uint32_t bid = book.GetBestBid();
    const uint32_t ask = book.GetBestAsk();

    if (bid != 1000 || ask != 0) {
        std::cerr << "Smoke test failed: expected bid=1000 ask=0, got bid=" << bid << " ask=" << ask << '\n';
        return false;
    }

    std::cout << "\n[orderbook smoke test] result: PASS\n";
    return true;
}

} // namespace cacheline::tests
