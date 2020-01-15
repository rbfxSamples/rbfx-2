//
// Copyright (c) 2008-2019 the Urho3D project.
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

#include "../Glow/BakedChunkVicinity.h"

#include "../Glow/LightTracer.h"
#include "../IO/Log.h"

#include <EASTL/sort.h>

namespace Urho3D
{

namespace
{

/// Calculate frustum containing all shadow casters for given volume and light direction.
Frustum CalculateDirectionalLightFrustum(const BoundingBox& boundingBox,
    const Vector3& lightDirection, float distance, float angle)
{
    const Quaternion rotation{ Vector3::DOWN, lightDirection };
    const float widthPadding = distance * Sin(angle);
    BoundingBox lightSpaceBoundingBox = boundingBox.Transformed(rotation.Inverse().RotationMatrix());
    lightSpaceBoundingBox.min_.x_ -= widthPadding;
    lightSpaceBoundingBox.min_.z_ -= widthPadding;
    lightSpaceBoundingBox.max_.x_ += widthPadding;
    lightSpaceBoundingBox.max_.z_ += widthPadding;
    lightSpaceBoundingBox.max_.y_ += distance;

    Frustum frustum;
    frustum.Define(lightSpaceBoundingBox, static_cast<Matrix3x4>(rotation.RotationMatrix()));
    return frustum;
}

}

BakedChunkVicinity CreateBakedChunkVicinity(Context* context,
    BakedSceneCollector& collector, const IntVector3& chunk,
    const LightmapSettings& lightmapSettings, const IncrementalLightmapperSettings& incrementalSettings)
{
    const BoundingBox lightReceiversBoundingBox = collector.GetChunkBoundingBox(chunk);
    const ea::vector<LightProbeGroup*> uniqueLightProbeGroups = collector.GetUniqueLightProbeGroups(chunk);
    const ea::vector<Light*> relevantLights = collector.GetLightsInBoundingBox(chunk, lightReceiversBoundingBox);
    const ea::vector<StaticModel*> uniqueStaticModels = collector.GetUniqueStaticModels(chunk);

    // Bake geometry buffers
    const LightmapGeometryBakingScenesArray geometryBakingScenes = GenerateLightmapGeometryBakingScenes(
        context, uniqueStaticModels, lightmapSettings.charting_.lightmapSize_, lightmapSettings.geometryBaking_);
    LightmapChartGeometryBufferVector geometryBuffers = BakeLightmapGeometryBuffers(geometryBakingScenes.bakingScenes_);

    ea::vector<unsigned> lightmapsInChunk;
    for (LightmapChartGeometryBuffer& geometryBuffer : geometryBuffers)
        lightmapsInChunk.push_back(geometryBuffer.index_);

    // Collect shadow casters for direct lighting
    ea::hash_set<StaticModel*> relevantStaticModels;
    for (Light* light : relevantLights)
    {
        if (light->GetLightType() == LIGHT_DIRECTIONAL)
        {
            const Vector3 direction = light->GetNode()->GetWorldDirection();
            const Frustum frustum = CalculateDirectionalLightFrustum(lightReceiversBoundingBox, direction,
                incrementalSettings.directionalLightShadowDistance_, 0.0f);
            const ea::vector<StaticModel*> shadowCasters = collector.GetStaticModelsInFrustum(chunk, frustum);
            relevantStaticModels.insert(shadowCasters.begin(), shadowCasters.end());
        }
        else
        {
            BoundingBox extendedBoundingBox = lightReceiversBoundingBox;
            extendedBoundingBox.Merge(light->GetNode()->GetWorldPosition());
            BoundingBox shadowCastersBoundingBox = light->GetWorldBoundingBox();
            shadowCastersBoundingBox.Clip(extendedBoundingBox);
            const ea::vector<StaticModel*> shadowCasters = collector.GetStaticModelsInBoundingBox(
                chunk, shadowCastersBoundingBox);
            relevantStaticModels.insert(shadowCasters.begin(), shadowCasters.end());
        }
    }

    // Collect light receivers for indirect lighting propagation
    BoundingBox indirectBoundingBox = lightReceiversBoundingBox;
    indirectBoundingBox.min_ -= Vector3::ONE * incrementalSettings.indirectPadding_;
    indirectBoundingBox.max_ += Vector3::ONE * incrementalSettings.indirectPadding_;

    const ea::vector<StaticModel*> indirectStaticModels = collector.GetStaticModelsInBoundingBox(
        chunk, indirectBoundingBox);
    relevantStaticModels.insert(indirectStaticModels.begin(), indirectStaticModels.end());

    // Collect light receivers, unique are first
    for (StaticModel* staticModel : uniqueStaticModels)
        relevantStaticModels.erase(staticModel);

    ea::vector<StaticModel*> staticModels = uniqueStaticModels;
    staticModels.insert(staticModels.end(), relevantStaticModels.begin(), relevantStaticModels.end());

    // Collect light probes, unique are first
    const ea::vector<LightProbeGroup*> lightProbesInVolume = collector.GetLightProbeGroupsInBoundingBox(
        chunk, indirectBoundingBox);
    ea::hash_set<LightProbeGroup*> relevantLightProbes(lightProbesInVolume.begin(), lightProbesInVolume.end());

    for (LightProbeGroup* group : uniqueLightProbeGroups)
        relevantLightProbes.erase(group);

    ea::vector<LightProbeGroup*> lightProbeGroups = uniqueLightProbeGroups;
    lightProbeGroups.insert(lightProbeGroups.end(), relevantLightProbes.begin(), relevantLightProbes.end());

    LightProbeCollection lightProbesCollection;
    LightProbeGroup::CollectLightProbes(lightProbeGroups, lightProbesCollection);

    // Create scene for raytracing
    const unsigned uvChannel = lightmapSettings.geometryBaking_.uvChannel_;
    const SharedPtr<RaytracerScene> raytracerScene = CreateRaytracingScene(context, staticModels, uvChannel);

    // Match raytracer geometries and geometry buffer
    ea::vector<RaytracerGeometry> raytracerGeometriesSorted = raytracerScene->GetGeometries();
    bool matching = geometryBakingScenes.idToObject_.size() <= raytracerGeometriesSorted.size() + 1;
    if (matching)
    {
        ea::sort(raytracerGeometriesSorted.begin(), raytracerGeometriesSorted.end(), CompareRaytracerGeometryByObject);
        for (unsigned i = 1; i < geometryBakingScenes.idToObject_.size(); ++i)
        {
            const RaytracerGeometry& raytracerGeometry = raytracerGeometriesSorted[i - 1];
            const GeometryIDToObjectMapping& mapping = geometryBakingScenes.idToObject_[i];
            if (raytracerGeometry.objectIndex_ != mapping.objectIndex_
                || raytracerGeometry.geometryIndex_ != mapping.geometryIndex_
                || raytracerGeometry.lodIndex_ != mapping.lodIndex_)
            {
                matching = false;
                break;
            }
        }
    }

    if (!matching)
    {
        for (LightmapChartGeometryBuffer& geometryBuffer : geometryBuffers)
            ea::fill(geometryBuffer.geometryIds_.begin(), geometryBuffer.geometryIds_.end(), 0u);

        URHO3D_LOGERROR("Cannot match raytracer geometries with lightmap G-Buffer");
    }

    ea::vector<unsigned> geometryBufferToRaytracerGeometry;
    geometryBufferToRaytracerGeometry.resize(geometryBakingScenes.idToObject_.size(), M_MAX_UNSIGNED);
    if (matching)
    {
        for (unsigned i = 1; i < geometryBakingScenes.idToObject_.size(); ++i)
            geometryBufferToRaytracerGeometry[i] = raytracerGeometriesSorted[i - 1].raytracerGeometryId_;
    }

    // Preprocess geometry buffers
    for (LightmapChartGeometryBuffer& geometryBuffer : geometryBuffers)
        PreprocessGeometryBuffer(geometryBuffer, *raytracerScene, geometryBufferToRaytracerGeometry, lightmapSettings.tracing_);

    // Collect lights
    ea::vector<BakedLight> bakedLights;
    for (Light* light : relevantLights)
    {
        Node* node = light->GetNode();
        BakedLight bakedLight;
        bakedLight.lightType_ = light->GetLightType();
        bakedLight.lightMode_ = light->GetLightMode();
        bakedLight.lightColor_ = light->GetEffectiveColor();
        bakedLight.position_ = node->GetWorldPosition();
        bakedLight.rotation_ = node->GetWorldRotation();
        bakedLight.direction_ = node->GetWorldDirection();
        bakedLights.push_back(bakedLight);
    }

    BakedChunkVicinity chunkVicinity;
    chunkVicinity.lightmaps_ = lightmapsInChunk;
    chunkVicinity.raytracerScene_ = raytracerScene;
    chunkVicinity.geometryBuffers_ = ea::move(geometryBuffers);
    chunkVicinity.geometryBufferToRaytracer_ = ea::move(geometryBufferToRaytracerGeometry);
    chunkVicinity.bakedLights_ = bakedLights;
    chunkVicinity.lightProbesCollection_ = lightProbesCollection;

    return chunkVicinity;
}

}
