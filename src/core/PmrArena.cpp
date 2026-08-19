#include "PmrArena.h"

// Linux headers, TODO: consider adding ifdef PLATFORM_LINUX 
#include <sys/mman.h>
#include <unistd.h>

namespace cacheline {

	PmrArena::PmrArena(size_t capacityBytes) noexcept
		: m_capacityBytes(capacityBytes) {}

	PmrArena::~PmrArena() {
		Release();
	}

	[[nodiscard]] bool PmrArena::Initialize() noexcept {
		if (m_rawBuffer != nullptr)
			return true;

// Platform dependent code start.
		m_rawBuffer = mmap(nullptr, m_capacityBytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);

		if (m_rawBuffer == MAP_FAILED) {
			m_rawBuffer = mmap(nullptr, m_capacityBytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		}

		if (m_rawBuffer == MAP_FAILED) return false;
// Platform dependent code end.

		auto* ptr = static_cast<volatile uint8_t*>(m_rawBuffer);
		for (size_t offset = 0; offset < m_capacityBytes; offset += 4096) {
			ptr[offset] = 0;
		}

		m_bufferResource = std::make_unique<std::pmr::monotonic_buffer_resource>(m_rawBuffer, m_capacityBytes, std::pmr::null_memory_resource());

		return true;
	}

	[[nodiscard]] std::pmr::memory_resource* PmrArena::GetResource() noexcept {
		return m_bufferResource.get();
	}

	void PmrArena::Reset() noexcept {
		if (m_bufferResource) {
			m_bufferResource->release();
			m_bufferResource = std::make_unique<std::pmr::monotonic_buffer_resource>(m_rawBuffer, m_capacityBytes, std::pmr::null_memory_resource());
		}
	}

	void PmrArena::Release() noexcept {
		if (!m_rawBuffer) 
			return;

		m_bufferResource.reset();

// Platform dependent code start.
		munmap(m_rawBuffer, m_capacityBytes);
// Platform dependent code end.
		m_rawBuffer = nullptr;
	}

} // namespace cacheline