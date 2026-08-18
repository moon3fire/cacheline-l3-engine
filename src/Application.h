#pragma once

#include <atomic>
#include <cstdint>
#include <memory_resource>
#include <thread>

//#include "core/..."

namespace cacheline {

	constexpr size_t DEFAULT_RING_BUFFER_CAPACITY = 1 << 16; // 65,536 <-> 2^16

	struct Config {
		uint32_t networkCoreID{ 1 };
		uint32_t engineCoreID{ 1 };
		size_t arenaSizeMb{ 256 };
		const char* network_interface{ "eth0" };
		uint16_t udp_port{ 12012 };
	};

	class Application {
	public:
		Application() noexcept;
		~Application();

		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;

		[[nodiscard]] bool Initialize(int argc, char* argv[]) noexcept;
		void Run() noexcept;
		void Stop() noexcept;
	
	private:
		bool SetupMemoryArena() noexcept;
		bool SetupNetworkDriver() noexcept;
		void PinCurrentThread(uint32_t currentCoreID) noexcept;

		void NetworkIngestLoop() noexcept;
		void OrderbookEngineLoop() noexcept;
	private:
		Config m_config{};
		static inline std::atomic<bool> m_running{ true };

		PmrArena m_memoryArena;
		SpscQueue<NetworkFrame, DEFAULT_RING_BUFFER_CAPACITY> m_frameQueue;

		NetworkDriver m_netDriver;
		L3OrderBook m_orderBook;

		LatencyProfiler m_profiler;

		std::thread m_netThread;
		std::thread m_engineThread;
	};

} // namespace cacheline