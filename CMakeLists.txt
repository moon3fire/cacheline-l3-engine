cmake_minimum_required(VERSION 3.25)

# Enable MSVC Hot Reload / Debug Information Format control
if (POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
        "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("cacheline-l3-engine" VERSION 1.0.0 LANGUAGES CXX C)

# Global C++20 enforcement
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for tooling (clangd, LSPs, VS Code)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory("src")