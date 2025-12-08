cmake_minimum_required(VERSION 3.15)
project(SDL3_Lua_Sol3 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# === SDL3 ===
# Try find_package first (works with vcpkg, manual installs)
find_package(SDL3 QUIET CONFIG)
if(NOT SDL3_FOUND)
    # Fallback to pkg-config (Linux)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL3 REQUIRED sdl3)
endif()

# === SDL3_ttf ===
# Try find_package first
find_package(SDL3_ttf QUIET CONFIG)
if(NOT SDL3_ttf_FOUND)
    # Fallback to pkg-config
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL3_TTF REQUIRED sdl3-ttf)
    else()
        message(FATAL_ERROR "SDL3_ttf not found. Install via vcpkg, brew, or system package manager.")
    endif()
endif()

# === LuaJIT ===
# Try multiple methods to find LuaJIT
# Method 1: Check for LuaJIT cmake config (vcpkg provides this)
find_package(LuaJIT QUIET CONFIG)
if(NOT LuaJIT_FOUND)
    # Method 2: Try unofficial-luajit (vcpkg naming)
    find_package(unofficial-luajit QUIET CONFIG)
    if(unofficial-luajit_FOUND)
        set(LUAJIT_FOUND TRUE)
        set(LUAJIT_TARGET unofficial::luajit::luajit)
    endif()
endif()

if(NOT LUAJIT_FOUND AND NOT unofficial-luajit_FOUND)
    # Method 3: Try pkg-config (Linux/macOS)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LUAJIT luajit)
    endif()
endif()

if(NOT LUAJIT_FOUND AND NOT unofficial-luajit_FOUND AND NOT LUAJIT_INCLUDE_DIRS)
    # Method 4: Manual search
    find_path(LUAJIT_INCLUDE_DIRS luajit.h
        PATH_SUFFIXES luajit luajit-2.1 luajit-2.0
        HINTS
            ${LUAJIT_DIR}/include
            $ENV{LUAJIT_DIR}/include
    )
    find_library(LUAJIT_LIBRARIES
        NAMES luajit-5.1 luajit lua51
        HINTS
            ${LUAJIT_DIR}/lib
            $ENV{LUAJIT_DIR}/lib
    )
    if(LUAJIT_INCLUDE_DIRS AND LUAJIT_LIBRARIES)
        set(LUAJIT_FOUND TRUE)
    else()
        message(FATAL_ERROR "LuaJIT not found. Install via vcpkg, brew, or system package manager.\n"
                "Or set LUAJIT_DIR to the LuaJIT installation directory.")
    endif()
endif()

# === Sol3 ===
# Sol3 is header-only, fetch automatically
include(FetchContent)
FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG v3.3.1
)
FetchContent_MakeAvailable(sol2)

# === Main executable ===
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/widgets/TextWidget.cpp
    src/graphics/FontManager.cpp
    src/events/EventHandler.cpp
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${LUAJIT_INCLUDE_DIRS}
)
if(DEFINED SDL3_TTF_INCLUDE_DIRS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${SDL3_TTF_INCLUDE_DIRS})
endif()

# Link libraries
if(TARGET SDL3::SDL3)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
elseif(DEFINED SDL3_LIBRARIES)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL3_LIBRARIES})
endif()

if(TARGET SDL3_ttf::SDL3_ttf)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3_ttf::SDL3_ttf)
elseif(DEFINED SDL3_TTF_LIBRARIES)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL3_TTF_LIBRARIES})
endif()

if(TARGET unofficial::luajit::luajit)
    target_link_libraries(${PROJECT_NAME} PRIVATE unofficial::luajit::luajit)
elseif(DEFINED LUAJIT_LIBRARIES)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${LUAJIT_LIBRARIES})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE sol2::sol2)

# === Post-build: Copy assets ===
# Copy Lua scripts to build directory
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/scripts
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/scripts
    COMMENT "Copying Lua scripts to build directory"
)

# Copy assets to build directory (fonts, etc.)
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    COMMENT "Copying assets to build directory"
)

# === Platform-specific settings ===
if(WIN32)
    # Copy DLLs on Windows (vcpkg handles this automatically with VCPKG_APPLOCAL_DEPS)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE FALSE  # Keep as console app for Lua print output
    )
endif()

if(APPLE)
    # macOS-specific settings
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE FALSE
    )
endif()
