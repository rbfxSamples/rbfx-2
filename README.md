![rbfx-logo](https://user-images.githubusercontent.com/19151258/57008846-a292be00-6bfb-11e9-8303-d79e6dd36038.png)

![Windows](https://dev.azure.com/rbfx/rbfx/_apis/build/status/rokups.rbfx?branchName=master&jobName=Windows&label=Windows) ![Linux](https://dev.azure.com/rbfx/rbfx/_apis/build/status/rokups.rbfx?branchName=master&jobName=Linux&label=Linux) ![MacOS](https://dev.azure.com/rbfx/rbfx/_apis/build/status/rokups.rbfx?branchName=master&jobName=MacOS&label=MacOS) ![Web](https://dev.azure.com/rbfx/rbfx/_apis/build/status/rokups.rbfx?branchName=master&jobName=Web&label=Web) ![Android](https://dev.azure.com/rbfx/rbfx/_apis/build/status/rokups.rbfx?branchName=master&jobName=Android&label=Android) ![iOS](https://dev.azure.com/rbfx/rbfx/_apis/build/status/rokups.rbfx?branchName=master&jobName=iOS&label=iOS)

[![Join the chat at https://gitter.im/rokups/rbfx](https://badges.gitter.im/rokups/rbfx.svg)](https://gitter.im/rokups/rbfx?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

**rbfx** is a free lightweight, cross-platform 2D and 3D game engine implemented in C++ and released under the MIT license. Greatly inspired by OGRE and Horde3D.

This project is a fork of [urho3d.github.io](http://urho3d.github.io/).

## License

Licensed under the MIT license, see [LICENSE](https://github.com/urho3d/Urho3D/blob/master/LICENSE) for details.

## Features overview

* Multiple rendering API support
  * Direct3D9
  * Direct3D11
  * OpenGL 2.0 or 3.2
  * OpenGL ES 2.0 or 3.0
  * WebGL
* HLSL or GLSL shaders + caching of HLSL bytecode
* Configurable rendering pipeline with default implementations of
  * Forward
  * Light pre-pass
  * Deferred
* Component based scene model
* Skeletal (with hardware skinning), vertex morph and node animation
* Automatic instancing on SM3 capable hardware
* Point, spot and directional lights
* Shadow mapping for all light types
  * Cascaded shadow maps for directional lights
  * Normal offset adjustment in addition to depth bias
* Particle rendering
* Geomipmapped terrain
* Static and skinned decals
* Auxiliary view rendering (reflections etc.)
* Geometry, material & animation LOD
* Software rasterized occlusion culling
* Post-processing
* HDR rendering and PBR rendering
* 2D sprites and particles that integrate into the 3D scene
* Task-based multithreading
* Hierarchical performance profiler
* Scene and object load/save in binary, XML and JSON formats
* Keyframe animation of object attributes
* Background loading of resources
* Keyboard, mouse, joystick and touch input (if available)
* Cross-platform support using SDL 2.0
* Physics using Bullet
* 2D physics using Box2D
* Scripting using C#
* Networking using SLikeNet + possibility to make HTTP requests
* Pathfinding and crowd simulation using Recast/Detour
* Inverse kinematics
* Image loading using stb_image + DDS / KTX / PVR compressed texture support + WEBP image format support
* 2D and “3D” audio playback, Ogg Vorbis support using stb_vorbis + WAV format support
* TrueType font rendering using FreeType
* Unicode string support
* Inbuilt UI, localization
* WYSIWYG scene editor and UI-layout editor implemented with undo & redo capabilities and hot code reload
* Model/scene/animation/material import from formats supported by Open Asset Import Library
* Alternative model/animation import from OGRE mesh.xml and skeleton.xml files
* Supported IDEs: Visual Studio, Xcode, Eclipse, CodeBlocks, CodeLite, QtCreator, CLion
* Supported compiler toolchains: MSVC, GCC, Clang, MinGW, and their cross-compiling derivatives
* Supports both 32-bit and 64-bit build
* Build as single external library (can be linked against statically or dynamically)
* ImGui integration used in tools

## Screenshots

![editor](https://user-images.githubusercontent.com/19151258/49943614-09376980-fef1-11e8-88fe-8c26fcf30a59.jpg)

## Dependencies

rbfx uses the following third-party libraries:
- Box2D 2.3.2 WIP (http://box2d.org)
- Bullet 2.86.1 (http://www.bulletphysics.org)
- Civetweb 1.7 (https://github.com/civetweb/civetweb)
- FreeType 2.8 (https://www.freetype.org)
- GLEW 1.13.0 (http://glew.sourceforge.net)
- SLikeNet (https://github.com/SLikeSoft/SLikeNet)
- libcpuid 0.4.0+ (https://github.com/anrieff/libcpuid)
- LZ4 1.7.5 (https://github.com/lz4/lz4)
- MojoShader (https://icculus.org/mojoshader)
- Open Asset Import Library 4.1.0 (http://assimp.sourceforge.net)
- pugixml 1.9 (http://pugixml.org)
- rapidjson 1.1.0 (https://github.com/miloyip/rapidjson)
- Recast/Detour (https://github.com/recastnavigation/recastnavigation)
- SDL 2.0.10+ (https://www.libsdl.org)
- StanHull (https://codesuppository.blogspot.com/2006/03/john-ratcliffs-code-suppository-blog.html)
- stb_image 2.18 (https://nothings.org)
- stb_image_write 1.08 (https://nothings.org)
- stb_rect_pack 0.11 (https://nothings.org)
- stb_textedit 1.9 (https://nothings.org)
- stb_truetype 1.15 (https://nothings.org)
- stb_vorbis 1.13b (https://nothings.org)
- WebP (https://chromium.googlesource.com/webm/libwebp)
- ETCPACK (https://github.com/Ericsson/ETCPACK)
- imgui 1.74 (https://github.com/ocornut/imgui/tree/docking)
- tracy (https://bitbucket.org/wolfpld/tracy/)
- nativefiledialog (https://github.com/mlabbe/nativefiledialog)
- IconFontCppHeaders (https://github.com/juliettef/IconFontCppHeaders)
- ImGuizmo 1.61 (https://github.com/CedricGuillemet/ImGuizmo)
- crunch (https://github.com/Unity-Technologies/crunch/)
- CLI11 v1.5.1 (https://github.com/CLIUtils/CLI11/)
- fmt 6.0.0 (http://fmtlib.net/)
- spdlog 1.3.1 (https://github.com/gabime/spdlog)
- EASTL 3.13.04 (https://github.com/electronicarts/EASTL/)
- SWIG 4.0 (http://swig.org/)

rbfx optionally uses the following external third-party libraries:
- Mono (http://www.mono-project.com/download/stable/)

## Supported Platforms

| Graphics API/Platform | Windows | Linux | MacOS | iOS | Android | Web |
| --------------------- |:-------:|:-----:|:-----:|:---:|:-------:|:---:|
| D3D9 / D3D11          | ✔       |       |       |     |         |     |
| OpenGL 2 / 3.1        | ✔       | ✔     | ✔     |     |         |     |
| OpenGL ES 2 / 3       |         |       |       | ✔   | ✔       |     |
| WebGL                 |         |       |       |     |         | ✔   |

![](https://dev.azure.com/rbfx/rbfx/_apis/build/status/rokups.rbfx?branchName=master&jobName=Windows&configuration=Windows%20static-msvc-d3d11&label=static-msvc-d3d11)
![](https://dev.azure.com/rbfx/rbfx/_apis/build/status/rokups.rbfx?branchName=master&jobName=Windows&configuration=Windows%20shared-msvc-d3d11&label=shared-msvc-d3d11)
![](https://dev.azure.com/rbfx/rbfx/_apis/build/status/rokups.rbfx?branchName=master&jobName=Windows&configuration=Windows%20static-mingw-d3d9&label=static-mingw-d3d9)
![](https://dev.azure.com/rbfx/rbfx/_apis/build/status/rokups.rbfx?branchName=master&jobName=Windows&configuration=Windows%20shared-mingw-d3d9&label=shared-mingw-d3d9)
![](https://dev.azure.com/rbfx/rbfx/_apis/build/status/rokups.rbfx?branchName=master&jobName=Linux&configuration=Linux%20static-gcc-opengl&label=static-gcc-opengl)
![](https://dev.azure.com/rbfx/rbfx/_apis/build/status/rokups.rbfx?branchName=master&jobName=Linux&configuration=Linux%20shared-gcc-opengl&label=shared-gcc-opengl)
![](https://dev.azure.com/rbfx/rbfx/_apis/build/status/rokups.rbfx?branchName=master&jobName=Linux&configuration=Linux%20static-clang-opengl&label=static-clang-opengl)
![](https://dev.azure.com/rbfx/rbfx/_apis/build/status/rokups.rbfx?branchName=master&jobName=Linux&configuration=Linux%20shared-clang-opengl&label=shared-clang-opengl)
![](https://dev.azure.com/rbfx/rbfx/_apis/build/status/rokups.rbfx?branchName=master&jobName=MacOS&configuration=MacOS%20static-clang-opengl&label=static-macos-clang-opengl)
![](https://dev.azure.com/rbfx/rbfx/_apis/build/status/rokups.rbfx?branchName=master&jobName=MacOS&configuration=MacOS%20shared-clang-opengl&label=shared-macos-clang-opengl)
![](https://dev.azure.com/rbfx/rbfx/_apis/build/status/rokups.rbfx?branchName=master&jobName=Web&label=web)
![](https://dev.azure.com/rbfx/rbfx/_apis/build/status/rokups.rbfx?branchName=master&jobName=Android&label=android)
![](https://dev.azure.com/rbfx/rbfx/_apis/build/status/rokups.rbfx?branchName=master&jobName=iOS&label=ios)
