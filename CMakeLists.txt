cmake_minimum_required(VERSION 3.20)
project(VulkanHelloWorld LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use vcpkg toolchain if VCPKG_ROOT is set
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

# Find packages
find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(freetype CONFIG REQUIRED)

# Find glslc shader compiler from Vulkan SDK
if(DEFINED ENV{VULKAN_SDK})
    set(GLSLC_PATH "$ENV{VULKAN_SDK}/Bin/glslc.exe")
    if(NOT EXISTS "${GLSLC_PATH}")
        message(WARNING "glslc not found at ${GLSLC_PATH}. Shaders may not compile.")
        set(GLSLC "")
    else()
        set(GLSLC "${GLSLC_PATH}")
    endif()
else()
    find_program(GLSLC glslc)
endif()

if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found. Install Vulkan SDK or set VULKAN_SDK environment variable.")
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Vulkan_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/main.cpp
    src/Window.cpp
    src/VulkanContext.cpp
    src/FontRenderer.cpp
)

# Header files (for IDE)
set(HEADERS
    include/Window.hpp
    include/VulkanContext.hpp
    include/FontRenderer.hpp
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    Vulkan::Vulkan
    freetype
)

# Shader compilation
set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/compiled_shaders)

file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

set(VERTEX_SHADER_SRC ${SHADER_SOURCE_DIR}/text.vert)
set(FRAGMENT_SHADER_SRC ${SHADER_SOURCE_DIR}/text.frag)
set(VERTEX_SHADER_DST ${SHADER_BINARY_DIR}/text.vert.spv)
set(FRAGMENT_SHADER_DST ${SHADER_BINARY_DIR}/text.frag.spv)

add_custom_command(
    OUTPUT ${VERTEX_SHADER_DST}
    COMMAND ${GLSLC} ${VERTEX_SHADER_SRC} -o ${VERTEX_SHADER_DST}
    DEPENDS ${VERTEX_SHADER_SRC}
    COMMENT "Compiling vertex shader"
)

add_custom_command(
    OUTPUT ${FRAGMENT_SHADER_DST}
    COMMAND ${GLSLC} ${FRAGMENT_SHADER_SRC} -o ${FRAGMENT_SHADER_DST}
    DEPENDS ${FRAGMENT_SHADER_SRC}
    COMMENT "Compiling fragment shader"
)

add_custom_target(shaders ALL DEPENDS ${VERTEX_SHADER_DST} ${FRAGMENT_SHADER_DST})
add_dependencies(${PROJECT_NAME} shaders)

# Copy compiled shaders to executable directory after build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy ${VERTEX_SHADER_DST} $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders/
    COMMAND ${CMAKE_COMMAND} -E copy ${FRAGMENT_SHADER_DST} $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders/
    COMMENT "Copying shaders to output directory"
)

# Copy font to executable directory if present (optional)
if(EXISTS ${CMAKE_SOURCE_DIR}/fonts/Roboto-Regular.ttf)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/fonts
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/fonts/Roboto-Regular.ttf
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/fonts/Roboto-Regular.ttf
        COMMENT "Copying Roboto-Regular.ttf to output directory"
    )
else()
    message(STATUS "Optional font file fonts/Roboto-Regular.ttf not found. Will use system font fallback.")
endif()

# Copy Vulkan DLL on Windows if using vcpkg
if(WIN32 AND DEFINED ENV{VULKAN_SDK})
    set(VULKAN_DLL_PATH "$ENV{VULKAN_SDK}/Bin/vulkan-1.dll")
    if(EXISTS ${VULKAN_DLL_PATH})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${VULKAN_DLL_PATH}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMENT "Copying vulkan-1.dll"
        )
    else()
        message(WARNING "Vulkan DLL not found at ${VULKAN_DLL_PATH}; skipping copy.")
    endif()
endif()
