project(Diligent)

set (CSD ${CMAKE_CURRENT_SOURCE_DIR})

## DiligentPrimitives
file(GLOB SOURCE_CODE Primitives/src/*)
add_library(DiligentPrimitives STATIC ${SOURCE_CODE})
target_include_directories(DiligentPrimitives PUBLIC $<BUILD_INTERFACE:${CSD}/Primitives/interface>)

## DiligentPlatform
file(GLOB SOURCE_CODE Platforms/Basic/src/*)
add_library(DiligentPlatform STATIC ${SOURCE_CODE})
target_include_directories(DiligentPlatform PUBLIC $<BUILD_INTERFACE:${CSD}/Platforms/interface> $<BUILD_INTERFACE:${CSD}/Platforms/Basic/interface>)
target_link_libraries(DiligentPlatform PUBLIC DiligentPrimitives)

# PLATFORM_WIN32 PLATFORM_UNIVERSAL_WINDOWS PLATFORM_ANDROID PLATFORM_LINUX PLATFORM_MACOS PLATFORM_IOS PLATFORM_TVOS PLATFORM_EMSCRIPTEN
if (UWP)
    file(GLOB SOURCE_CODE Platforms/UWP/src/*)
    target_include_directories(DiligentPlatform PUBLIC $<BUILD_INTERFACE:${CSD}/Platforms/UWP/interface>)
    target_compile_definitions(DiligentPlatform PUBLIC -DPLATFORM_UNIVERSAL_WINDOWS=1)
elseif (WIN32)
    file(GLOB SOURCE_CODE Platforms/Win32/src/*)
    target_include_directories(DiligentPlatform PUBLIC $<BUILD_INTERFACE:${CSD}/Platforms/Win32/interface>)
    target_compile_definitions(DiligentPlatform PUBLIC -DPLATFORM_WIN32=1)
elseif (ANDROID)
    file(GLOB SOURCE_CODE Platforms/Android/src/*)
    target_include_directories(DiligentPlatform PUBLIC $<BUILD_INTERFACE:${CSD}/Platforms/Android/interface>)
    target_compile_definitions(DiligentPlatform PUBLIC -DPLATFORM_ANDROID=1)
elseif (LINUX)
    file(GLOB SOURCE_CODE Platforms/Linux/src/*)
    target_include_directories(DiligentPlatform PRIVATE Platforms/Linux/include PUBLIC $<BUILD_INTERFACE:${CSD}/Platforms/Linux/interface>)
    target_compile_definitions(DiligentPlatform PUBLIC -DPLATFORM_LINUX=1)
elseif (APPLE)
    file(GLOB SOURCE_CODE Platforms/Apple/src/*)
    target_include_directories(DiligentPlatform PUBLIC $<BUILD_INTERFACE:${CSD}/Platforms/Apple/interface>)
    if (TVOS)
        target_compile_definitions(DiligentPlatform PUBLIC -DPLATFORM_TVOS=1)
    elseif (IOS)
        target_compile_definitions(DiligentPlatform PUBLIC -DPLATFORM_IOS=1)
    else ()
        target_compile_definitions(DiligentPlatform PUBLIC -DPLATFORM_MACOS=1)
    endif ()
elseif (WEB)
    file(GLOB SOURCE_CODE Platforms/Emscripten/src/*)
    target_include_directories(DiligentPlatform PUBLIC $<BUILD_INTERFACE:${CSD}/Platforms/Emscripten/interface>)
    target_compile_definitions(DiligentPlatform PUBLIC -DPLATFORM_EMSCRIPTEN=1)
else ()
    message(FATAL_ERROR "Unsupported platform")
endif ()
target_sources(DiligentPlatform PRIVATE ${SOURCE_CODE})

## DiligentCommon
file(GLOB SOURCE_CODE Common/src/*)
add_library(DiligentCommon STATIC ${SOURCE_CODE})
target_include_directories(DiligentCommon PRIVATE Common/include PUBLIC $<BUILD_INTERFACE:${CSD}/Common/interface>)
target_link_libraries(DiligentCommon PUBLIC DiligentPlatform)
if (NOT WIN32)
    target_compile_definitions(DiligentCommon PUBLIC "-D__forceinline=__always_inline")
endif ()

## Graphics API support.
if (URHO3D_D3D11)
    set (D3D11_SUPPORTED ON)
elseif (URHO3D_OPENGL)
    if (WEB OR ANDROID)
        set (GLES_SUPPORTED ON)
    else ()
        set (GL_SUPPORTED ON)
    endif ()
endif ()
#if (WIN32)
#    set (D3D11_SUPPORTED ON)
#    set (D3D12_SUPPORTED ON)
#endif ()
#if (DESKTOP AND NOT APPLE)
#    set (VULKAN_SUPPORTED ON)
#    set (GL_SUPPORTED ON)
#endif ()
#if (WEB OR ANDROID)
#    set (GLES_SUPPORTED ON)
#    if (ANDROID)
#        set (VULKAN_SUPPORTED ON)
#    endif ()
#endif ()
#if (APPLE)
#    set (METAL_SUPPORTED ON)
#endif ()
foreach (D D3D11_SUPPORTED GL_SUPPORTED GLES_SUPPORTED)# D3D12_SUPPORTED VULKAN_SUPPORTED  METAL_SUPPORTED)
    if (${${D}})
        list (APPEND DILIGENT_GRAPHICS_API_DEFINES -D${D}=1)
    endif ()
endforeach ()

## DiligentGraphics
file(GLOB SOURCE_CODE Graphics/GraphicsEngine/src/*)
add_library(DiligentGraphics STATIC ${SOURCE_CODE})
file(GLOB SOURCE_CODE Graphics/GraphicsAccessories/src/*)
target_sources(DiligentGraphics PRIVATE ${SOURCE_CODE})
target_link_libraries(DiligentGraphics PUBLIC DiligentCommon)
target_include_directories(DiligentGraphics PUBLIC $<BUILD_INTERFACE:${CSD}/Graphics/GraphicsAccessories/interface> $<BUILD_INTERFACE:${CSD}/Graphics/GraphicsEngine/interface> $<BUILD_INTERFACE:${CSD}/Graphics/GraphicsEngine/include>)
target_compile_definitions(DiligentGraphics PUBLIC ${DILIGENT_GRAPHICS_API_DEFINES} DILIGENT_NO_HLSL=1 DILIGENT_NO_GLSLANG=1)
set(SOURCE_CODE
    Graphics/Archiver/src/ArchiveRepacker.cpp
    Graphics/Archiver/src/ArchiverFactory.cpp
    Graphics/Archiver/src/ArchiverImpl.cpp
    Graphics/Archiver/src/SerializationDeviceImpl.cpp
    Graphics/Archiver/src/SerializedPipelineStateImpl.cpp
    Graphics/Archiver/src/SerializedRenderPassImpl.cpp
    Graphics/Archiver/src/SerializedResourceSignatureImpl.cpp
    Graphics/Archiver/src/SerializedShaderImpl.cpp
    Graphics/Archiver/src/Archiver_Inc.hpp
)
target_sources(DiligentGraphics PRIVATE ${SOURCE_CODE})
target_include_directories(DiligentGraphics PUBLIC $<BUILD_INTERFACE:${CSD}/Graphics/Archiver/interface> $<BUILD_INTERFACE:${CSD}/Graphics/Archiver/include>)

## GraphicsTools
file(GLOB SOURCE_CODE Graphics/GraphicsTools/src/*.cpp)
if (NOT GL_SUPPORTED AND NOT GLES_SUPPORTED)
    list (REMOVE_ITEM SOURCE_CODE Graphics/GraphicsTools/src/TextureUploaderGL.cpp)
else ()
    target_include_directories(DiligentGraphics PRIVATE Graphics/GraphicsEngineOpenGL/interface)
endif ()
if (NOT D3D11_SUPPORTED)
    list (REMOVE_ITEM SOURCE_CODE Graphics/GraphicsTools/src/TextureUploaderD3D11.cpp)
else ()
    target_include_directories(DiligentGraphics PRIVATE Graphics/GraphicsEngineD3D11/interface Graphics/GraphicsEngineD3DBase/include)
endif ()
if (NOT D3D12_SUPPORTED AND NOT VULKAN_SUPPORTED)
    list (REMOVE_ITEM SOURCE_CODE Graphics/GraphicsTools/src/TextureUploaderD3D12_Vk.cpp Graphics/GraphicsEngineD3DBase/include)
else ()
    target_include_directories(DiligentGraphics PRIVATE Graphics/GraphicsEngineD3D12/interface Graphics/GraphicsEngineVulkan/interface)
endif ()
target_sources(DiligentGraphics PRIVATE ${SOURCE_CODE} ThirdParty/xxHash/xxhash.c ThirdParty/xxHash/xxh_x86dispatch.c)
target_include_directories(DiligentGraphics PUBLIC $<BUILD_INTERFACE:${CSD}/Graphics/GraphicsTools/interface> $<BUILD_INTERFACE:${CSD}/ThirdParty/xxHash>)

## ShaderTools
target_sources(DiligentGraphics PRIVATE Graphics/ShaderTools/src/ShaderToolsCommon.cpp Graphics/ShaderTools/src/DXILUtilsStub.cpp)
if (D3D11_SUPPORTED)
    target_sources(DiligentGraphics PRIVATE ThirdParty/GPUOpenShaderUtils/DXBCChecksum.cpp ${CSD}/Graphics/ShaderTools/src/DXBCUtils.cpp ${CSD}/Graphics/ShaderTools/src/HLSLUtils.cpp)
endif ()
target_include_directories(DiligentGraphics PUBLIC $<BUILD_INTERFACE:${CSD}/Graphics/ShaderTools/include>)

## DiligentGraphicsOpenGL
if (URHO3D_OPENGL AND (GL_SUPPORTED OR GLES_SUPPORTED))
    file(GLOB SOURCE_CODE Graphics/GraphicsEngineOpenGL/src/*)
    list(FILTER SOURCE_CODE EXCLUDE REGEX ".+/src/.+(Android|Emscripten|IOS|MacOS|Linux|Windows)\\.(cpp|mm)")
    list(FILTER SOURCE_CODE EXCLUDE REGEX ".+/src/.+(GLImpl|GLESImpl)\\.(cpp|mm)")
    if (GLES_SUPPORTED)
        list (APPEND SOURCE_CODE Graphics/GraphicsEngineOpenGL/src/RenderDeviceGLESImpl.cpp)
    else ()
        list (APPEND SOURCE_CODE Graphics/GraphicsEngineOpenGL/src/RenderDeviceGLImpl.cpp)
    endif ()
    if (NOT IOS)
        list (APPEND SOURCE_CODE Graphics/GraphicsEngineOpenGL/src/SwapChainGLImpl.cpp)
    endif ()
    if (ANDROID)
        list (APPEND SOURCE_CODE Graphics/GraphicsEngineOpenGL/src/GLContextAndroid.cpp Graphics/GraphicsEngineOpenGL/src/GLStubsAndroid.cpp)
    elseif (WEB)
        list (APPEND SOURCE_CODE Graphics/GraphicsEngineOpenGL/src/GLContextEmscripten.cpp)
    elseif (WIN32)
        list (APPEND SOURCE_CODE Graphics/GraphicsEngineOpenGL/src/GLContextWindows.cpp)
    elseif (IOS)
        list (APPEND SOURCE_CODE Graphics/GraphicsEngineOpenGL/src/GLContextIOS.mm Graphics/GraphicsEngineOpenGL/src/SwapChainGLIOS.mm)
    elseif (APPLE)
        list (APPEND SOURCE_CODE Graphics/GraphicsEngineOpenGL/src/GLContextMacOS.mm)
    elseif (LINUX)
        list (APPEND SOURCE_CODE Graphics/GraphicsEngineOpenGL/src/GLContextLinux.cpp)
    endif ()
    add_library(DiligentGraphicsOpenGL STATIC ${SOURCE_CODE} Graphics/Archiver/src/Archiver_GL.cpp)
    target_include_directories(DiligentGraphicsOpenGL PUBLIC $<BUILD_INTERFACE:${CSD}>
        PRIVATE Graphics/GraphicsEngineOpenGL/interface Graphics/GraphicsEngineOpenGL/include Graphics/GraphicsEngine/include
    )
    target_link_libraries(DiligentGraphicsOpenGL PUBLIC DiligentGraphics GLEW)
endif ()

if (D3D11_SUPPORTED)   # TODO
    file(GLOB SOURCE_CODE Graphics/GraphicsEngineD3DBase/src/*)
    add_library(DiligentGraphicsD3DBase STATIC ${SOURCE_CODE})
    target_include_directories(DiligentGraphicsD3DBase PUBLIC $<BUILD_INTERFACE:${CSD}/Graphics/GraphicsEngineD3DBase/interface> $<BUILD_INTERFACE:${CSD}/Graphics/GraphicsEngineD3DBase/include>)
    target_link_libraries(DiligentGraphicsD3DBase PUBLIC DiligentGraphics)

    file(GLOB_RECURSE SOURCE_CODE ThirdParty/DirectXShaderCompiler/*.cpp)
    target_sources(DiligentGraphicsD3DBase PRIVATE ${SOURCE_CODE})
    target_include_directories(DiligentGraphicsD3DBase PUBLIC $<BUILD_INTERFACE:${CSD}/ThirdParty/DirectXShaderCompiler>)
endif ()

if (URHO3D_D3D11 AND D3D11_SUPPORTED)
    file(GLOB SOURCE_CODE Graphics/GraphicsEngineD3D11/src/*)
    add_library(DiligentGraphicsD3D11 STATIC ${SOURCE_CODE} Graphics/Archiver/src/Archiver_D3D11.cpp)
    target_include_directories(DiligentGraphicsD3D11 PUBLIC $<BUILD_INTERFACE:${CSD}>
        PRIVATE Graphics/GraphicsEngineD3D11/interface Graphics/GraphicsEngineD3D11/include Graphics/GraphicsEngine/include
    )
    target_link_libraries(DiligentGraphicsD3D11 PUBLIC DiligentGraphicsD3DBase)
endif ()

if (NOT URHO3D_MERGE_STATIC_LIBS)
    install(TARGETS DiligentGraphics DiligentCommon DiligentPlatform DiligentPrimitives EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG})
    foreach (T DiligentGraphicsOpenGL DiligentGraphicsD3DBase DiligentGraphicsD3D11)
        if (TARGET ${T})
            install(TARGETS ${T} EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG})
        endif ()
    endforeach ()
endif ()

#    Graphics/Archiver/src/Archiver_D3D11.cpp
#    Graphics/Archiver/src/Archiver_D3D12.cpp
#    Graphics/Archiver/src/Archiver_Mtl.mm
#    Graphics/Archiver/src/Archiver_Vk.cpp
