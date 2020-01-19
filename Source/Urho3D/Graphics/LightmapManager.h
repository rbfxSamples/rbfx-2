//
// Copyright (c) 2019-2020 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

/// \file

#pragma once

#include "../Graphics/LightmapSettings.h"
#include "../Scene/Component.h"

namespace Urho3D
{

/// Lightmap manager component.
class URHO3D_API LightmapManager : public Component
{
    URHO3D_OBJECT(LightmapManager, Component);

public:
    /// Construct.
    explicit LightmapManager(Context* context);
    /// Destruct.
    ~LightmapManager() override;
    /// Register object factory. Drawable must be registered first.
    static void RegisterObject(Context* context);

    /// Bake lightmaps in main thread.
    void Bake();

private:
    /// Lightmap baking settings.
    LightmapSettings lightmapSettings_;
    /// Incremental baking settings.
    IncrementalLightmapperSettings incrementalBakingSettings_;
    /// Whether the baking is scheduled.
    bool bakingScheduled_{};
};

}
