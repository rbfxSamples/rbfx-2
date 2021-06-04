#
# Copyright (c) 2017-2020 the rbfx project.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
project(TheForge)

file (GLOB_RECURSE SOURCE_FILES_COMMON
    TheForgeInterfaces.cpp
    OS/Core/*
    OS/Interfaces/*
    OS/Logging/*
    OS/Math/*
    OS/FileSystem/FileSystem.cpp
    OS/FileSystem/SystemRun.cpp
    OS/MemoryTracking/*
    ThirdParty/OpenSource/basis_universal/transcoder/basisu_transcoder.cpp
)
file (GLOB SOURCE_FILES_COMMON_2
    OS/*
	Renderer/*
)

add_library(TheForge STATIC ${SOURCE_FILES_COMMON} ${SOURCE_FILES_COMMON_2})
target_link_libraries(TheForge PUBLIC EASTL)
target_include_directories(TheForge SYSTEM PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
if (WIN32)
    file (GLOB_RECURSE SOURCE_FILES_PLATFORM
        OS/Windows/*
        Renderer/Direct3D11/*
        ThirdParty/OpenSource/gpudetect/src/*
    )
    target_compile_definitions(TheForge PRIVATE -D_WINDOWS=1 -DTF_USE_DIRECT3D11)
    if (MSVC AND NOT UWP)
        list (APPEND SOURCE_FILES_PLATFORM OS/Windows/WindowsStackTraceDump.cpp OS/Windows/WindowsStackTraceDump.h)
    endif ()
    if (NOT MINGW AND URHO3D_64BIT)
        file (GLOB_RECURSE SOURCE_FILES_PLATFORM_D3D12 Renderer/Direct3D12/*)
        list (APPEND SOURCE_FILES_PLATFORM ${SOURCE_FILES_PLATFORM_D3D12})
        target_link_libraries(TheForge PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/OpenSource/DirectXShaderCompiler/lib/x64/dxcompiler.lib)
        target_compile_definitions(TheForge PRIVATE -DTF_USE_DIRECT3D12)
    endif ()
elseif (LINUX)
    file (GLOB_RECURSE SOURCE_FILES_PLATFORM OS/Linux/* OS/FileSystem/UnixFileSystem.cpp)
    target_compile_definitions(TheForge PRIVATE)
elseif (ANDROID)
    file (GLOB_RECURSE SOURCE_FILES_PLATFORM
        Renderer/OpenGLES/*
        OS/Android/*
        OS/FileSystem/UnixFileSystem.cpp
    )
    target_compile_definitions(TheForge PRIVATE -DGLES)
    target_link_libraries(TheForge PUBLIC EGL)
elseif (IOS)
    file (GLOB_RECURSE SOURCE_FILES_PLATFORM
        Renderer/Metal/*
        OS/Darwin/*
        OS/Linux/LinuxTime.cpp
    )
    target_compile_definitions(TheForge PUBLIC -DTARGET_IOS=1 PRIVATE -DMETAL)
elseif (APPLE)
    file (GLOB_RECURSE SOURCE_FILES_PLATFORM
        Renderer/Metal/*
        OS/Darwin/*
        OS/Linux/LinuxTime.cpp
    )
    target_compile_definitions(TheForge PRIVATE -DMETAL)
elseif (WEB)
    message(FATAL_ERROR "Not supported.")
endif ()
target_sources(TheForge PRIVATE ${SOURCE_FILES_PLATFORM} ${SOURCE_FILES_VULKAN})

if (NOT APPLE)
    target_sources(TheForge PRIVATE
        Tools/SpirvTools/SpirvTools.h
        Tools/SpirvTools/SpirvTools.cpp
    )
    target_link_libraries(spirv-cross-core)
endif ()

if ((WIN32 OR LINUX OR ANDROID) AND Vulkan_FOUND AND NOT UWP)
    file (GLOB_RECURSE SOURCE_FILES_VULKAN Renderer/Vulkan/*)
    target_sources(TheForge PRIVATE ${SOURCE_FILES_VULKAN})
    target_compile_definitions(TheForge PRIVATE -DTF_USE_VULKAN)
    target_link_libraries(TheForge PUBLIC Vulkan::Vulkan)
endif ()

target_compile_definitions(TheForge PRIVATE -DEASTL_EXPORTS -DEASTDC_EXPORTS)
if (WIN32)
    target_compile_definitions(TheForge PRIVATE -DTF_DLL_EXPORT PUBLIC -DTF_DLL_IMPORT)
endif ()

if (NOT URHO3D_MERGE_STATIC_LIBS)
    install(TARGETS TheForge EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG})
endif ()
