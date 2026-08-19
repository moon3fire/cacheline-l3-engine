#pragma once

#include <cstdint>
#include <bit>

namespace cacheline::protocol
{

// fast inline byte swapping for big-endian ITCH network data
#if defined(__GNUC__) || defined(__clang__)
    inline uint16_t BSwap16(uint16_t val) noexcept { return __builtin_bswap16(val); }
    inline uint32_t BSwap32(uint32_t val) noexcept { return __builtin_bswap32(val); }
    inline uint64_t BSwap64(uint64_t val) noexcept { return __builtin_bswap64(val); }
#else
    inline uint16_t BSwap16(uint16_t val) noexcept { return (val >> 8) | (val << 8); }
    inline uint32_t BSwap32(uint32_t val) noexcept
    {
        return ((val << 24) & 0xFF000000) | ((val << 8) & 0x00FF0000) |
               ((val >> 8) & 0x0000FF00) | ((val >> 24) & 0x000000FF);
    }
    inline uint64_t BSwap64(uint64_t val) noexcept
    {
        return ((val << 56) & 0xFF00000000000000ULL) |
               ((val << 40) & 0x00FF000000000000ULL) |
               ((val << 24) & 0x0000FF0000000000ULL) |
               ((val << 8) & 0x000000FF00000000ULL) |
               ((val >> 8) & 0x00000000FF000000ULL) |
               ((val >> 24) & 0x0000000000FF0000ULL) |
               ((val >> 40) & 0x000000000000FF00ULL) |
               ((val >> 56) & 0x00000000000000FFULL);
    }
#endif

#pragma pack(push, 1)

    struct Timestamp48
    {
        uint8_t bytes[6];

        [[nodiscard]] uint64_t GetNanoseconds() const noexcept
        {
            return (static_cast<uint64_t>(bytes[0]) << 40) |
                   (static_cast<uint64_t>(bytes[1]) << 32) |
                   (static_cast<uint64_t>(bytes[2]) << 24) |
                   (static_cast<uint64_t>(bytes[3]) << 16) |
                   (static_cast<uint64_t>(bytes[4]) << 8) |
                   (static_cast<uint64_t>(bytes[5]));
        }
    };

    enum class ItchMessageType : char
    {
        SystemEvent = 'S',
        StockDirectory = 'R',
        StockTradingAction = 'H',
        AddOrderNoMPID = 'A',
        AddOrderMPID = 'F',
        OrderExecuted = 'E',
        OrderExecutedWithPrice = 'C',
        OrderCancel = 'X',
        OrderDelete = 'D',
        OrderReplace = 'U',
        TradeNonCross = 'P',
        CrossTrade = 'Q',
        BrokenTrade = 'B'
    };

    struct ItchHeader
    {
        char messageType;
        uint16_t stockLocate;
        uint16_t trackingNumber;
        Timestamp48 timestamp;
    };

    struct AddOrderMsg
    {
        char messageType; // 'A'
        uint16_t stockLocate;
        uint16_t trackingNumber;
        Timestamp48 timestamp;
        uint64_t orderReferenceNumber;
        char buySellIndicator; // 'B' or 'S'
        uint32_t shares;
        char stock[8];
        uint32_t price; // Fixed point (4 decimals)

        [[nodiscard]] uint64_t GetOrderID() const noexcept { return BSwap64(orderReferenceNumber); }
        [[nodiscard]] uint32_t GetShares() const noexcept { return BSwap32(shares); }
        [[nodiscard]] uint32_t GetPrice() const noexcept { return BSwap32(price); }
    };

    struct AddOrderMPIDMsg
    {
        char messageType; // 'F'
        uint16_t stockLocate;
        uint16_t trackingNumber;
        Timestamp48 timestamp;
        uint64_t orderReferenceNumber;
        char buySellIndicator; // 'B' or 'S'
        uint32_t shares;
        char stock[8];
        uint32_t price; // Fixed point (4 decimals)
        char mpid[4];

        [[nodiscard]] uint64_t GetOrderID() const noexcept { return BSwap64(orderReferenceNumber); }
        [[nodiscard]] uint32_t GetShares() const noexcept { return BSwap32(shares); }
        [[nodiscard]] uint32_t GetPrice() const noexcept { return BSwap32(price); }
    };

    struct OrderExecutedMsg
    {
        char messageType; // 'E'
        uint16_t stockLocate;
        uint16_t trackingNumber;
        Timestamp48 timestamp;
        uint64_t orderReferenceNumber;
        uint32_t executedShares;
        uint64_t matchNumber;

        [[nodiscard]] uint64_t GetOrderID() const noexcept { return BSwap64(orderReferenceNumber); }
        [[nodiscard]] uint32_t GetExecutedShares() const noexcept { return BSwap32(executedShares); }
    };

    struct OrderExecutedWithPriceMsg
    {
        char messageType; // 'C'
        uint16_t stockLocate;
        uint16_t trackingNumber;
        Timestamp48 timestamp;
        uint64_t orderReferenceNumber;
        uint32_t executedShares;
        uint64_t matchNumber;
        char printable; // 'N' or 'Y'
        uint32_t executionPrice;

        [[nodiscard]] uint64_t GetOrderID() const noexcept { return BSwap64(orderReferenceNumber); }
        [[nodiscard]] uint32_t GetExecutedShares() const noexcept { return BSwap32(executedShares); }
        [[nodiscard]] uint32_t GetPrice() const noexcept { return BSwap32(executionPrice); }
    };

    struct OrderCancelMsg
    {
        char messageType; // 'X'
        uint16_t stockLocate;
        uint16_t trackingNumber;
        Timestamp48 timestamp;
        uint64_t orderReferenceNumber;
        uint32_t canceledShares;

        [[nodiscard]] uint64_t GetOrderID() const noexcept { return BSwap64(orderReferenceNumber); }
        [[nodiscard]] uint32_t GetCanceledShares() const noexcept { return BSwap32(canceledShares); }
    };

    struct OrderDeleteMsg
    {
        char messageType; // 'D'
        uint16_t stockLocate;
        uint16_t trackingNumber;
        Timestamp48 timestamp;
        uint64_t orderReferenceNumber;

        [[nodiscard]] uint64_t GetOrderID() const noexcept { return BSwap64(orderReferenceNumber); }
    };

    // 'U': Order Replace - 35 bytes total
    struct OrderReplaceMsg
    {
        char messageType; // 'U'
        uint16_t stockLocate;
        uint16_t trackingNumber;
        Timestamp48 timestamp;
        uint64_t originalOrderReferenceNumber;
        uint64_t newOrderReferenceNumber;
        uint32_t shares;
        uint32_t price;

        [[nodiscard]] uint64_t GetOldOrderID() const noexcept { return BSwap64(originalOrderReferenceNumber); }
        [[nodiscard]] uint64_t GetNewOrderID() const noexcept { return BSwap64(newOrderReferenceNumber); }
        [[nodiscard]] uint32_t GetShares() const noexcept { return BSwap32(shares); }
        [[nodiscard]] uint32_t GetPrice() const noexcept { return BSwap32(price); }
    };

#pragma pack(pop)

    static_assert(sizeof(ItchHeader) == 11, "ItchHeader layout mismatch");
    static_assert(sizeof(AddOrderMsg) == 36, "AddOrderMsg layout mismatch");
    static_assert(sizeof(AddOrderMPIDMsg) == 40, "AddOrderMPIDMsg layout mismatch");
    static_assert(sizeof(OrderExecutedMsg) == 31, "OrderExecutedMsg layout mismatch");
    static_assert(sizeof(OrderExecutedWithPriceMsg) == 36, "OrderExecutedWithPriceMsg layout mismatch");
    static_assert(sizeof(OrderCancelMsg) == 23, "OrderCancelMsg layout mismatch");
    static_assert(sizeof(OrderDeleteMsg) == 19, "OrderDeleteMsg layout mismatch");
    static_assert(sizeof(OrderReplaceMsg) == 35, "OrderReplaceMsg layout mismatch");

} // namespace cacheline::protocol