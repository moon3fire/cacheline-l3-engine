add_executable(cacheline-l3-engine
    main.cpp
    # Add engine sources here
)

# Place build outputs into a visible folder under the repository so
# Visual Studio's Solution Explorer can show the produced binaries.
set_target_properties(cacheline-l3-engine PROPERTIES
    # Explicit per-configuration runtime directories so Visual Studio can
    # reliably locate the built executable for Debug/Release when using
    # the single-config Ninja generator.
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/out/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/out/bin/Release"
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/out/lib/Debug"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/out/lib/Release"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/out/lib/Debug"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/out/lib/Release"
)

target_include_directories(cacheline-l3-engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Windows (MSVC)
if (MSVC)
    target_compile_options(cacheline-l3-engine PRIVATE
        /W4
        /permissive-
        /utf-8
        $<$<CONFIG:Release>:/O2 /Ob2 /Oi /Ot /arch:AVX2>
    )
    target_compile_definitions(cacheline-l3-engine PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
    # Winsock2 & Registered I/O libraries
    target_link_libraries(cacheline-l3-engine PRIVATE ws2_32 mswsock)

# Linux & macOS (GCC / Clang / AppleClang)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(cacheline-l3-engine PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        $<$<CONFIG:Release>:-O3 -march=native -flto>
    )

    if (APPLE)
        target_compile_definitions(cacheline-l3-engine PRIVATE PLATFORM_MACOS)
    elseif (UNIX AND NOT APPLE)
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
    endif()
endif()