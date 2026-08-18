#include "PmrArena.h"

// Linux headers, TODO: consider adding ifdef PLATFORM_LINUX 
#include <sys/mman.h>
#include <unistd.h>


namespace cacheline {

	explicit PmrArena::PmrArena(size_t capacityBytes) noexcept 
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

		}
// Platform dependent code end.

	}

} // namespace cacheline