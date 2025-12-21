cmake_minimum_required(VERSION 3.18)
project(quran-renderer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Disable problematic warnings (from Skia headers and compatibility issues)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wno-missing-braces -Wno-sign-compare -Wno-double-promotion -Wno-error)
endif()

option(BUILD_SHARED_LIBS "Build shared library" ON)
option(BUILD_ANDROID "Build for Android" OFF)

# Quran text data path (required)
set(QURAN_TEXT_DIR "/home/ali/Desktop/Projects/mushaf-android/libs/visualmetafont/src/qurantext" CACHE PATH "Path to qurantext source directory")

# Source files - include quran text sources
set(CORE_SOURCES
    src/core/quran_renderer.cpp
    src/core/hb_skia_canvas.cpp
    ${QURAN_TEXT_DIR}/quran.cpp
    ${QURAN_TEXT_DIR}/surahs.cpp
)

# Headers
set(PUBLIC_HEADERS
    include/quran/renderer.h
)

# Create library
add_library(quran_renderer ${CORE_SOURCES})

target_include_directories(quran_renderer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
        ${QURAN_TEXT_DIR}
)

# HarfBuzz dependency (link statically to include custom justify functions)
if(HARFBUZZ_INCLUDE_DIR AND HARFBUZZ_LIBRARY_DIR)
    target_include_directories(quran_renderer PRIVATE
        ${HARFBUZZ_INCLUDE_DIR}
        ${HARFBUZZ_INCLUDE_DIR}/src
    )
    
    # Use static library if available, otherwise shared
    if(EXISTS "${HARFBUZZ_LIBRARY_DIR}/libharfbuzz.a")
        target_link_libraries(quran_renderer PRIVATE "${HARFBUZZ_LIBRARY_DIR}/libharfbuzz.a")
    else()
        target_link_directories(quran_renderer PRIVATE ${HARFBUZZ_LIBRARY_DIR})
        target_link_libraries(quran_renderer PRIVATE harfbuzz)
    endif()
else()
    find_package(harfbuzz REQUIRED)
    target_link_libraries(quran_renderer PRIVATE harfbuzz::harfbuzz)
endif()

# Skia dependency (link statically)
if(SKIA_INCLUDE_DIR AND SKIA_LIBRARY_DIR)
    target_include_directories(quran_renderer PRIVATE 
        ${SKIA_INCLUDE_DIR}
        ${SKIA_INCLUDE_DIR}/include/core
        ${SKIA_INCLUDE_DIR}/include/config
        ${SKIA_INCLUDE_DIR}/include/private
        ${SKIA_INCLUDE_DIR}/include/private/base
        ${SKIA_INCLUDE_DIR}/src/core
        ${SKIA_INCLUDE_DIR}/src/base
    )
    
    # Use static library if available
    if(EXISTS "${SKIA_LIBRARY_DIR}/libskia.a")
        target_link_libraries(quran_renderer PRIVATE "${SKIA_LIBRARY_DIR}/libskia.a")
    else()
        target_link_directories(quran_renderer PRIVATE ${SKIA_LIBRARY_DIR})
        target_link_libraries(quran_renderer PRIVATE skia)
    endif()
else()
    find_package(skia REQUIRED)
    target_link_libraries(quran_renderer PRIVATE skia::skia)
endif()

# Additional libraries that Skia may need
target_link_libraries(quran_renderer PRIVATE
    pthread
    dl
)

# Set output name
set_target_properties(quran_renderer PROPERTIES
    OUTPUT_NAME "quranrenderer"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# Export header
target_compile_definitions(quran_renderer PRIVATE QURAN_RENDERER_EXPORTS)

# Install rules
include(GNUInstallDirs)

install(TARGETS quran_renderer
    EXPORT quran-renderer-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES ${PUBLIC_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/quran
)

# Export package config
install(EXPORT quran-renderer-targets
    FILE quran-renderer-targets.cmake
    NAMESPACE QuranRenderer::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/quran-renderer
)
