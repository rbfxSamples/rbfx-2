//
// Copyright (c) 2017-2020 the rbfx project.
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

#pragma once

#include "../Graphics/Light.h"
#include "../Math/SphericalHarmonics.h"

#include <EASTL/fixed_vector.h>
#include <EASTL/sort.h>

namespace Urho3D
{

class ReflectionProbeData;

/// Common parameters for light accumulation.
struct LightAccumulatorContext
{
    unsigned maxVertexLights_{ 4 };
    unsigned maxPixelLights_{ 1 };
    LightImportance lightImportance_{};
    unsigned lightIndex_{};
    /// Array of lights to be indexed.
    const ea::vector<Light*>* lights_;
};

/// Accumulated light for forward rendering.
struct LightAccumulator
{
    /// Hints used for small buffer optimization
    /// @{
    static const unsigned MaxPixelLights = 4;
    static const unsigned MaxVertexLights = 4;
    static const unsigned NumElements = ea::max(MaxPixelLights + 1, 4u) + MaxVertexLights;
    /// @}

    using LightData = ea::pair<float, unsigned>;
    using LightContainer = ea::fixed_vector<LightData, NumElements>;
    using VertexLightContainer = ea::array<unsigned, MaxVertexLights>;

    /// Reset lights.
    void ResetLights()
    {
        lights_.clear();
        firstVertexLight_ = 0;
        numImportantLights_ = 0;
        numAutoLights_ = 0;
        vertexLightsHash_ = 0;
    }

    /// Accumulate light.
    void AccumulateLight(const LightAccumulatorContext& ctx, float penalty)
    {
        assert(vertexLightsHash_ == 0);

        if (ctx.lightImportance_ == LI_IMPORTANT)
            ++numImportantLights_;
        else if (ctx.lightImportance_ == LI_AUTO)
            ++numAutoLights_;

        // Add new light
        const auto lessPenalty = [&](const LightData& pair) { return penalty <= pair.first; };
        auto iter = ea::find_if(lights_.begin(), lights_.end(), lessPenalty);
        lights_.emplace(iter, penalty, ctx.lightIndex_);

        // First N important plus automatic lights are per-pixel
        firstVertexLight_ = ea::max(numImportantLights_,
            ea::min(numImportantLights_ + numAutoLights_, ctx.maxPixelLights_));

        // If too many lights, drop the least important one
        const unsigned maxLights = ctx.maxVertexLights_ + firstVertexLight_;
        if (lights_.size() > maxLights)
        {
            // TODO(renderer): Accumulate into SH
            lights_.pop_back();
        }
    }

    /// Cook light if necessary.
    void Cook()
    {
        if (vertexLightsHash_)
            return;

        const unsigned numLights = lights_.size();
        const auto compareIndex = [](const LightData& lhs, const LightData& rhs) { return lhs.second < rhs.second; };
        if (firstVertexLight_ < numLights)
            ea::sort(lights_.begin() + firstVertexLight_, lights_.end(), compareIndex);

        for (unsigned i = firstVertexLight_; i < numLights; ++i)
            CombineHash(vertexLightsHash_, (lights_[i].second + 1) * 2654435761);
        vertexLightsHash_ += !vertexLightsHash_;
    }

    /// Return light info, valid after cooking
    /// @{
    VertexLightContainer GetVertexLights() const
    {
        VertexLightContainer vertexLights;
        for (unsigned i = 0; i < 4; ++i)
        {
            const unsigned index = i + firstVertexLight_;
            vertexLights[i] = index < lights_.size() ? lights_.at(index).second : M_MAX_UNSIGNED;
        }
        return vertexLights;
    }

    ea::span<const LightData> GetPixelLights() const
    {
        return { lights_.data(), ea::min(static_cast<unsigned>(lights_.size()), firstVertexLight_) };
    }

    unsigned GetVertexLightsHash() const { return vertexLightsHash_; }
    /// @}

    /// Accumulated SH lights and ambient light.
    SphericalHarmonicsDot9 sphericalHarmonics_;
    /// Reflection probe.
    const ReflectionProbeData* reflectionProbe_{};

private:
    /// Container with per-pixel and per-vertex lights.
    LightContainer lights_;

    unsigned numImportantLights_{};
    unsigned numAutoLights_{};
    unsigned firstVertexLight_{};

    /// Hash of vertex lights. Non-zero.
    unsigned vertexLightsHash_{};
};

}
