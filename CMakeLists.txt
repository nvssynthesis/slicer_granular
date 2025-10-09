cmake_minimum_required(VERSION 3.15)

project(slicer-granular VERSION 0.4.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options for external dependencies
option(USE_SYSTEM_LIBRARIES "Use system-installed libraries instead of fetching" OFF)

# JUCE setup options
set(JUCE_DIR "" CACHE PATH "Path to JUCE framework (leave empty to use bundled JUCE in JUCE/ subdirectory)")
option(JUCE_FETCH_IF_MISSING "Automatically fetch JUCE if JUCE_DIR is not set and JUCE/ subdirectory doesn't exist" ON)
set(JUCE_VERSION "8.0.10" CACHE STRING "JUCE version to fetch if JUCE_FETCH_IF_MISSING is ON")

# External dependencies setup
include(FetchContent)

# fmt library
if(USE_SYSTEM_LIBRARIES)
    find_package(fmt REQUIRED)
else()
    FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 11.2.0
    )
    FetchContent_MakeAvailable(fmt)
endif()

# Xoshiro-cpp (header-only)
if(USE_SYSTEM_LIBRARIES AND DEFINED XOSHIRO_ROOT)
    set(XOSHIRO_INCLUDE_DIR ${XOSHIRO_ROOT})
else()
    FetchContent_Declare(
        Xoshiro
        GIT_REPOSITORY https://github.com/Reputeless/Xoshiro-cpp.git
        GIT_TAG master
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
    )
    FetchContent_MakeAvailable(Xoshiro)
    set(XOSHIRO_INCLUDE_DIR ${xoshiro_SOURCE_DIR})
endif()

# Handle JUCE setup with fallback logic
if(JUCE_DIR)
    # User specified their own JUCE path
    message(STATUS "Using user-specified JUCE from: ${JUCE_DIR}")
    add_subdirectory(${JUCE_DIR} juce_build)
    set(JUCE_SOURCE_DIR ${JUCE_DIR} PARENT_SCOPE)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    # Use bundled JUCE subdirectory (existing setup)
    message(STATUS "Using bundled JUCE from JUCE/ subdirectory")
    add_subdirectory(JUCE)
    set(JUCE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/JUCE" PARENT_SCOPE)
elseif(JUCE_FETCH_IF_MISSING)
    # Auto-fetch JUCE to the JUCE/ subdirectory
    message(STATUS "JUCE not found. Fetching JUCE ${JUCE_VERSION} to ${CMAKE_CURRENT_SOURCE_DIR}/JUCE...")

    set(JUCE_FETCH_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/JUCE")

    # Check if JUCE was already fetched (but git clone failed to complete)
    if(EXISTS "${JUCE_FETCH_LOCATION}" AND NOT EXISTS "${JUCE_FETCH_LOCATION}/CMakeLists.txt")
        message(STATUS "Removing incomplete JUCE directory...")
        file(REMOVE_RECURSE "${JUCE_FETCH_LOCATION}")
    endif()

    # Clone JUCE directly to the JUCE/ subdirectory
    if(NOT EXISTS "${JUCE_FETCH_LOCATION}/CMakeLists.txt")
        execute_process(
                COMMAND git clone --depth 1 --branch ${JUCE_VERSION} https://github.com/juce-framework/JUCE.git "${JUCE_FETCH_LOCATION}"
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                RESULT_VARIABLE GIT_CLONE_RESULT
                OUTPUT_VARIABLE GIT_CLONE_OUTPUT
                ERROR_VARIABLE GIT_CLONE_ERROR
        )

        if(NOT GIT_CLONE_RESULT EQUAL 0)
            message(STATUS "Git clone output: ${GIT_CLONE_OUTPUT}")
            message(STATUS "Git clone error: ${GIT_CLONE_ERROR}")
            message(FATAL_ERROR "Failed to clone JUCE repository")
        endif()

        message(STATUS "JUCE ${JUCE_VERSION} cloned successfully")
    endif()

    add_subdirectory(${JUCE_FETCH_LOCATION} juce_build)
    set(JUCE_SOURCE_DIR ${JUCE_FETCH_LOCATION} PARENT_SCOPE)
else()
    message(FATAL_ERROR
            "JUCE not found!\n"
            "Please either:\n"
            "  1. Set JUCE_DIR to your JUCE installation path, or\n"
            "  2. Place JUCE in the JUCE/ subdirectory, or\n"
            "  3. Enable JUCE_FETCH_IF_MISSING to auto-download JUCE"
    )
endif()

# Debug: Print JUCE version and status
message(STATUS "JUCE version: ${JUCE_VERSION}")
message(STATUS "JUCE found at: ${JUCE_SOURCE_DIR}")

# Create the plugin target
juce_add_plugin(slicer-granular
    COMPANY_NAME "Corrode Audio"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Crrd
    PLUGIN_CODE Slgr
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "slicer-granular"
    COMPANY_WEBSITE "https://nvssynthesis.github.io/"
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Synth
    AU_SANDBOX_SAFE TRUE
)

# Collect all source files from Source directory
file(GLOB_RECURSE SOURCE_FILES
    "Source/*.cpp"
    "Source/*.h"
)

# Add all source files
target_sources(slicer-granular PRIVATE ${SOURCE_FILES})

juce_generate_juce_header(slicer-granular)

# Add include directories
target_include_directories(slicer-granular
    SYSTEM PRIVATE
        nvs_libraries/nvs_libraries/external/sprout
        ${XOSHIRO_INCLUDE_DIR}
    PRIVATE
        Source
        nvs_libraries/nvs_libraries/include
)

# Link JUCE modules
target_link_libraries(slicer-granular
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_cryptography
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        fmt::fmt
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Compiler definitions
target_compile_definitions(slicer-granular
    PUBLIC
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:slicer-granular,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:slicer-granular,JUCE_VERSION>"
        _LIBCPP_ENABLE_CXX20_REMOVED_TYPE_TRAITS=1
)

# Platform-specific settings
if(APPLE)
    set_target_properties(slicer-granular PROPERTIES
        MACOSX_DEPLOYMENT_TARGET "13.1"
    )

    target_compile_options(slicer-granular PRIVATE -v)
endif()

# Build type specific definitions
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(slicer-granular PRIVATE NDEBUG=1)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(slicer-granular PRIVATE DEBUG=1 _DEBUG=1)
endif()