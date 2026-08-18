#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>
#include <utility>

namespace cacheline {

	template<typename T, size_t Capacity>
	class SpscQueue {
		static_assert((Capacity > 0) && ((Capacity & (Capacity - 1)) == 0),
			"SpscQueue capacity MUST be a power of 2!");
		
		
		static_assert(std::is_nothrow_move_constructible_v<T> || std::is_nothrow_copy_constructible_v<T>,
			"Type T must be nothrow movable/copyable for hot-path queueing!");

	public:
		SpscQueue() noexcept : m_head(0), m_tail(0) {}
		~SpscQueue() {
			T dummy;
			while (Pop(dummy));
		}

		SpscQueue(const SpscQueue&) = delete;
		SpscQueue& operator=(const SpscQueue&) = delete;

		
		template <typename... Args>
		[[nodiscard]] bool Emplace(Args&&... args) noexcept {
			const size_t currentTail = m_tail.load(std::memory_order_relaxed);
			const size_t currentHead = m_head.load(std::memory_order_acquire);

			if ((currentTail - currentHead) >= Capacity) [[unlikely]] {
				return false;
			}

			const size_t index = currentTail & MASK;

			new (&m_buffer[index].storage) T(std::forward<Args>(args)...);

			m_tail.store(currentTail + 1, std::memory_order_release);
			return true;
		}

		[[nodiscard]] bool Pop(T& value) noexcept {
			const size_t currentHead = m_head.load(std::memory_order_relaxed);
			const size_t currentTail = m_tail.load(std::memory_order_acquire);

			if (currentHead == currentTail) {
				return false;
			}

			const size_t index = currentHead & MASK;
			auto* slot_ptr = reinterpret_cast<T*>(&m_buffer[index].storage);

			value = std::move(*slot_ptr);
			slot_ptr->~T();

			m_head.store(currentHead + 1, std::memory_order_release);
			return true;
		}

		[[nodiscard]] bool Empty() const noexcept {
			return m_head.load(std::memory_order_relaxed) == m_tail.load(std::memory_order_relaxed);
		}

		[[nodiscard]] size_t GetSize() const noexcept {
			const size_t currentHead = m_head.load(std::memory_order_relaxed);
			const size_t currentTail = m_tail.load(std::memory_order_relaxed);
			return (currentTail >= currentHead) ? (currentTail - currentHead) : 0;
		}

		[[nodiscard]] static constexpr size_t GetCapacity() noexcept {
			return Capacity;
		}

	private:
		static constexpr size_t MASK = Capacity - 1;

		struct Slot {
			alignas(alignof(T)) uint8_t storage[sizeof(T)];
		};

		alignas(64) std::atomic<size_t> m_head{ 0 };
		alignas(64) std::atomic<size_t> m_tail{ 0 };
		alignas(64) Slot m_buffer[Capacity];
	};

} // namespace cacheline