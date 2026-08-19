add_executable(cacheline-l3-engine
    main.cpp
    "Application.h"
    "Application.cpp"
    "core/SpscQueue.h"
    "core/PmrArena.h"
    "core/PmrArena.cpp"
    "engine/L3OrderBook.h"
    "engine/L3OrderBook.cpp"
    "tests/OrderBookSmokeTest.h"
)

# Output directories
set_target_properties(cacheline-l3-engine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/lib"
)

target_include_directories(cacheline-l3-engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Compiler Flags (GCC / Clang)
target_compile_options(cacheline-l3-engine PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    $<$<CONFIG:Release>:-O3 -march=native -flto>
)

target_compile_definitions(cacheline-l3-engine PRIVATE PLATFORM_LINUX)

# POSIX threads & realtime library
target_link_libraries(cacheline-l3-engine PRIVATE pthread rt)

# Optional Linux AF_XDP / libxdp detection
find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
    pkg_check_modules(LIBXDP QUIET libxdp)
    if (LIBXDP_FOUND)
        target_include_directories(cacheline-l3-engine PRIVATE ${LIBXDP_INCLUDE_DIRS})
        target_link_libraries(cacheline-l3-engine PRIVATE ${LIBXDP_LIBRARIES})
        target_compile_definitions(cacheline-l3-engine PRIVATE HAS_AF_XDP)
    endif()
endif()