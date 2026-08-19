#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <memory_resource>
#include <new>
#include <span>
#include <utility>

namespace cacheline {

	class PmrArena {
	public:
		explicit PmrArena(size_t capacityBytes) noexcept;
		~PmrArena();

		PmrArena(const PmrArena&) = delete;
		PmrArena& operator=(const PmrArena&) = delete;

		[[nodiscard]] bool Initialize() noexcept;
		[[nodiscard]] std::pmr::memory_resource* GetResource() noexcept;

		void Reset() noexcept;
		[[nodiscard]] size_t GetCapacity() const noexcept;

	private:
		void Release() noexcept;

	private:
		size_t m_capacityBytes{ 0 };
		void* m_rawBuffer{ nullptr };
		std::unique_ptr<std::pmr::monotonic_buffer_resource> m_bufferResource{ nullptr };
	};

} // namespace cacheline