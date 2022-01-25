cmake_minimum_required(VERSION 3.20)
project(tm3-mod-exporter VERSION 0.1.0)

set(RELEASE_DIR "${CMAKE_SOURCE_DIR}/release")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

find_package(NVTT 3.1.6 REQUIRED)
find_package(OpenMP REQUIRED)

set(wxBUILD_SHARED OFF)

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)

FetchContent_Declare(
    wxWidgets
    GIT_REPOSITORY "https://github.com/wxWidgets/wxWidgets.git"
    GIT_TAG "v3.1.5"
)

FetchContent_MakeAvailable(wxWidgets)

set(CMAKE_CXX_STANDARD 20)

aux_source_directory(src sources)
add_executable(tm3-mod-exporter WIN32 ${sources})
target_link_libraries(tm3-mod-exporter OpenMP::OpenMP_CXX NVTT::NVTT wx::core wx::base)

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