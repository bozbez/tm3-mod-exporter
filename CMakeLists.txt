cmake_minimum_required(VERSION 3.20)
project(tm3-mod-exporter VERSION 0.1.0)

set(RELEASE_DIR "${CMAKE_SOURCE_DIR}/release")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_CXX_STANDARD 20)

find_package(NVTT 3.1.6 REQUIRED)
find_package(OpenMP REQUIRED)

include(FetchContent)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY "https://github.com/fmtlib/fmt.git"
    GIT_TAG "8.0.1"
)

FetchContent_Declare(
    CLI11
    GIT_REPOSITORY "https://github.com/CLIUtils/CLI11.git"
    GIT_TAG "v2.1.2"
)

FetchContent_MakeAvailable(fmt CLI11)

add_executable(tm3-mod-exporter src/tm3-mod-exporter.cpp)
target_link_libraries(tm3-mod-exporter OpenMP::OpenMP_CXX fmt::fmt CLI11::CLI11 NVTT::NVTT)

add_custom_command(TARGET tm3-mod-exporter POST_BUILD
    COMMAND if $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>==1 (
        "${CMAKE_COMMAND}" -E make_directory "${RELEASE_DIR}"
    )

    COMMAND if $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>==1 (
        "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>" "${RELEASE_DIR}"
    )

    COMMAND if $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>==1 (
        "${CMAKE_COMMAND}" -E copy "${NVTT_SHARED_LIBRARY}" "${RELEASE_DIR}"
    )

    COMMAND if $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>==1 (
        "${CMAKE_COMMAND}" -E copy "${NVTT_CUDA_SHARED_LIBRARY}" "${RELEASE_DIR}"
    )

    VERBATIM
)