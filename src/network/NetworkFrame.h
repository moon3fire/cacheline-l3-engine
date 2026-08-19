#pragma once

#include <cstdint>

namespace cacheline
{

    enum class MessageType : uint8_t
    {
        Add = 1,
        Cancel = 2,
        Modify = 3,
        Execute = 4
    };

    enum class Side : uint8_t
    {
        Buy = 0,
        Sell = 1
    };

    struct alignas(64) NetworkFrame
    {
        uint64_t timestampNS{0}; // hw/kernel receive timestamp
        uint64_t orderID{0};     // L3 order reference
        uint64_t sequenceNum{0}; // Feed sequence number for gap detection
        uint32_t price{0};
        uint32_t qty{0};
        MessageType msgType{MessageType::Add};
        Side side{Side::Buy};
        uint8_t _padding[26]{0}; // 64 byte padding
    };

    static_assert(sizeof(NetworkFrame) == 64, "NetworkFrame must be 64 bytes.");

} // namespace cacheline