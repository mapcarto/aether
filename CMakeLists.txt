cmake_minimum_required(VERSION 3.14)
project(aetoer
    VERSION 0.1.0
    DESCRIPTION "High-performance spatiotemporal event-driven engine"
    LANGUAGES C
)

# ---- C standard -----------------------------------------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ---- Build type defaults --------------------------------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---- Compiler flags -------------------------------------------------------
set(AE_WARN_FLAGS
    -Wall -Wextra -Wpedantic
    -Wno-unused-parameter
    -Wmissing-prototypes
    -Wstrict-prototypes
)

set(AE_RELEASE_FLAGS
    -O3
    -march=native
    -fomit-frame-pointer
    -funroll-loops
)

set(AE_DEBUG_FLAGS
    -g3
    -fsanitize=address,undefined
    -fno-omit-frame-pointer
)

# ---- Library target -------------------------------------------------------
add_library(aetoer STATIC
    src/ae_memory.c
    src/ae_ecs.c
    src/ae_spatial.c
    src/ae_event.c
)

target_include_directories(aetoer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_options(aetoer PRIVATE ${AE_WARN_FLAGS})

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(aetoer PRIVATE ${AE_RELEASE_FLAGS})
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(aetoer PRIVATE ${AE_DEBUG_FLAGS})
    target_link_options(aetoer PUBLIC -fsanitize=address,undefined)
endif()

target_link_libraries(aetoer PUBLIC m)

# ---- Tests ----------------------------------------------------------------
enable_testing()

find_package(Threads REQUIRED)

function(ae_test name)
    add_executable(${name} tests/${name}.c)
    target_link_libraries(${name} PRIVATE aetoer Threads::Threads)
    target_compile_options(${name} PRIVATE ${AE_WARN_FLAGS})
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${name} PRIVATE ${AE_DEBUG_FLAGS})
        target_link_options(${name} PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME ${name} COMMAND ${name})
endfunction()

ae_test(test_memory)
ae_test(test_ecs)
ae_test(test_spatial)
ae_test(test_event)
ae_test(test_perf)

# ---- Install --------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS aetoer
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/aetoer
    FILES_MATCHING PATTERN "*.h"
)

# ---- Summary --------------------------------------------------------------
message(STATUS "")
message(STATUS "  aetoer v${PROJECT_VERSION}")
message(STATUS "  Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install dir: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
