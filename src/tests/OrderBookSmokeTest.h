#pragma once

#include "engine/L3OrderBook.h"
#include "io/InputProcessor.h"
#include "network/NetworkFrame.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace cacheline::tests {
namespace {

[[nodiscard]] inline std::vector<Trade> FeedInput(L3OrderBook &book, const std::string &input) noexcept {
    SpscQueue<NetworkFrame, 256> inputQueue;
    InputProcessor<256> inputProcessor(inputQueue);
    std::istringstream stream(input);

    inputProcessor.RunFromStream(stream);

    std::vector<Trade> trades;
    NetworkFrame frame{};
    while (inputQueue.Pop(frame)) {
        const auto frameTrades = book.ProcessOrder(frame);
        trades.insert(trades.end(), frameTrades.begin(), frameTrades.end());
    }

    return trades;
}

[[nodiscard]] inline bool ScenarioCrossingAndPartialFill() noexcept {
    std::cout << "[scenario] crossing and partial fill\n";
    PmrArena arena(256u * 1024u * 1024u);
    if (!arena.Initialize()) {
        std::cerr << "scenario failed: arena init\n";
        return false;
    }

    L3OrderBook book(arena);
    const std::string input =
        "3 SELL 11 1010 5 ADD\n"
        "3 BUY 21 1011 1 ADD\n"
        "3 BUY 22 1012 1 ADD\n"
        "3 BUY 23 1015 1 ADD\n"
        "3 BUY 24 1020 5 ADD\n";

    const auto trades = FeedInput(book, input);
    if (trades.size() != 4) {
        std::cerr << "expected 4 trades, got " << trades.size() << '\n';
        return false;
    }

    const std::vector<uint32_t> expectedQtys{1u, 1u, 1u, 2u};
    for (size_t i = 0; i < trades.size(); ++i) {
        if (trades[i].symbol != 3 || trades[i].qty != expectedQtys[i] || trades[i].price != 1010) {
            std::cerr << "crossing scenario mismatch at trade " << i << '\n';
            return false;
        }
    }

    std::cout << "  trade totals: 1 + 1 + 1 + 2 = 5\n";
    return true;
}

[[nodiscard]] inline bool ScenarioNoCross() noexcept {
    std::cout << "[scenario] no cross when price is worse\n";
    PmrArena arena(256u * 1024u * 1024u);
    if (!arena.Initialize()) {
        std::cerr << "scenario failed: arena init\n";
        return false;
    }

    L3OrderBook book(arena);
    const auto trades = FeedInput(book,
        "3 SELL 11 1012 1 ADD\n"
        "3 BUY 12 1009 2 ADD\n");

    if (!trades.empty()) {
        std::cerr << "no-cross scenario produced trades unexpectedly\n";
        return false;
    }

    if (book.GetBestAsk(3) != 1012 || book.GetBestBid(3) != 1009) {
        std::cerr << "no-cross scenario left wrong best prices\n";
        return false;
    }

    return true;
}

[[nodiscard]] inline bool ScenarioCancelAndModify() noexcept {
    std::cout << "[scenario] cancel and modify\n";
    PmrArena arena(256u * 1024u * 1024u);
    if (!arena.Initialize()) {
        std::cerr << "scenario failed: arena init\n";
        return false;
    }

    L3OrderBook book(arena);
    const std::string input =
        "0 BUY 1 1000 50 ADD\n"
        "0 BUY 1 1000 75 MODIFY\n"
        "0 SELL 2 1010 40 ADD\n"
        "0 SELL 2 1010 0 CANCEL\n";

    const auto trades = FeedInput(book, input);
    if (!trades.empty()) {
        std::cerr << "cancel/modify scenario unexpectedly produced trades\n";
        return false;
    }

    if (book.GetBestBid(0) != 1000 || book.GetBestAsk(0) != 0) {
        std::cerr << "cancel/modify scenario produced wrong book state\n";
        return false;
    }

    return true;
}

[[nodiscard]] inline bool ScenarioMultiSymbolIsolation() noexcept {
    std::cout << "[scenario] multi-symbol isolation\n";
    PmrArena arena(256u * 1024u * 1024u);
    if (!arena.Initialize()) {
        std::cerr << "scenario failed: arena init\n";
        return false;
    }

    L3OrderBook book(arena);
    const auto trades = FeedInput(book,
        "1 SELL 10 1100 30 ADD\n"
        "2 BUY 20 1200 35 ADD\n");

    if (!trades.empty()) {
        std::cerr << "multi-symbol scenario should not cross across symbols\n";
        return false;
    }

    if (book.GetBestAsk(1) != 1100 || book.GetBestBid(2) != 1200) {
        std::cerr << "multi-symbol isolation mismatch\n";
        return false;
    }

    return true;
}

} // namespace

[[nodiscard]] inline bool RunOrderBookSmokeTest() noexcept {
    std::cout << "[orderbook trade tests] start\n";

    const bool ok =
        ScenarioCrossingAndPartialFill() &&
        ScenarioNoCross() &&
        ScenarioCancelAndModify() &&
        ScenarioMultiSymbolIsolation();

    if (!ok) {
        std::cerr << "[orderbook trade tests] result: FAIL\n";
        return false;
    }

    std::cout << "[orderbook trade tests] result: PASS\n";
    return true;
}

} // namespace cacheline::tests
