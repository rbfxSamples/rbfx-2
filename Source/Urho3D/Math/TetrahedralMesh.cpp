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

#include "../Precompiled.h"

#include "../IO/ArchiveSerialization.h"
#include "../IO/Log.h"
#include "../Math/Plane.h"
#include "../Math/TetrahedralMesh.h"

#include <EASTL/numeric.h>
#include <EASTL/sort.h>

namespace Urho3D
{

void TetrahedralMesh::Define(ea::span<const Vector3> positions)
{
    BoundingBox boundingBox(positions.data(), positions.size());
    const Vector3 size = boundingBox.Size();
    const float maxSize = ea::max({ 1.0f, size.x_, size.y_, size.z_ });
    // TODO(glow): Revise this place
    boundingBox.min_ -= Vector3::ONE;// * maxSize / 2;
    boundingBox.max_ += Vector3::ONE;// * maxSize / 2;
    InitializeSuperMesh(boundingBox);
    BuildTetrahedrons(positions);
}

void TetrahedralMesh::CollectEdges(ea::vector<ea::pair<unsigned, unsigned>>& edges)
{
    edges.clear();

    // Collect everything
    for (unsigned tetIndex = 0; tetIndex < numInnerTetrahedrons_; ++tetIndex)
    {
        const Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];
        for (unsigned i = 0; i < 4; ++i)
        {
            for (unsigned j = 0; j < 4; ++j)
            {
                const unsigned startIndex = tetrahedron.indices_[i];
                const unsigned endIndex = tetrahedron.indices_[j];
                edges.emplace_back(startIndex, endIndex);
            }
        }
    }

    // Sort edges and remove duplicates
    for (auto& edge : edges)
    {
        if (edge.first > edge.second)
            ea::swap(edge.first, edge.second);
    }

    ea::sort(edges.begin(), edges.end());
    edges.erase(ea::unique(edges.begin(), edges.end()), edges.end());
}

HighPrecisionSphere TetrahedralMesh::GetTetrahedronCircumsphere(unsigned tetIndex) const
{
    const Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];
    const HighPrecisionVector3 p0{ vertices_[tetrahedron.indices_[0]] };
    const HighPrecisionVector3 p1{ vertices_[tetrahedron.indices_[1]] };
    const HighPrecisionVector3 p2{ vertices_[tetrahedron.indices_[2]] };
    const HighPrecisionVector3 p3{ vertices_[tetrahedron.indices_[3]] };
    const HighPrecisionVector3 u1 = p1 - p0;
    const HighPrecisionVector3 u2 = p2 - p0;
    const HighPrecisionVector3 u3 = p3 - p0;

    const double d01 = u1.LengthSquared();
    const double d02 = u2.LengthSquared();
    const double d03 = u3.LengthSquared();

    const HighPrecisionVector3 u2u3 = u2.CrossProduct(u3);
    const HighPrecisionVector3 u3u1 = u3.CrossProduct(u1);
    const HighPrecisionVector3 u1u2 = u1.CrossProduct(u2);

    const HighPrecisionVector3 radiusNum = u2u3 * d01 + u3u1 * d02 + u1u2 * d03;
    const double radiusDen = 2 * u1.DotProduct(u2u3);

    if (Abs(radiusDen) < M_EPSILON * M_EPSILON)
    {
        URHO3D_LOGWARNING("Degenerate tetrahedron in tetrahedral mesh due to error in tetrahedral mesh generation");
        return HighPrecisionSphere{ {}, M_LARGE_VALUE * M_LARGE_VALUE };
    }

    const HighPrecisionVector3 center = p0 + radiusNum * (1.0 / radiusDen);

    // Radius is the minimum distance
    const double squaredDistances[4] = {
        (p0 - center).LengthSquared(),
        (p1 - center).LengthSquared(),
        (p2 - center).LengthSquared(),
        (p3 - center).LengthSquared()
    };

    const double radiusSquared = *ea::min_element(ea::begin(squaredDistances), ea::end(squaredDistances));
    return { center, Sqrt(radiusSquared) };
}

void TetrahedralMesh::InitializeSuperMesh(const BoundingBox& boundingBox)
{
    static const Vector3 offsets[NumSuperMeshVertices] =
    {
        { 0.0f, 0.0f, 0.0f }, // 0: 1st corner tetrahedron
        { 1.0f, 0.0f, 0.0f }, // 1:
        { 0.0f, 1.0f, 0.0f }, // 2:
        { 1.0f, 1.0f, 0.0f }, // 3: 2nd corner tetrahedron
        { 0.0f, 0.0f, 1.0f }, // 4:
        { 1.0f, 0.0f, 1.0f }, // 5: 3rh corner tetrahedron
        { 0.0f, 1.0f, 1.0f }, // 6: 4th corner tetrahedron
        { 1.0f, 1.0f, 1.0f }  // 7:
    };

    // TODO(glow): Use Tetrahedron here
    static const unsigned numTetrahedrons = 5;
    static const unsigned indices[numTetrahedrons][4] =
    {
        { 0, 1, 2, 4 }, // 1st corner tetrahedron
        { 3, 1, 2, 7 }, // 2nd corner tetrahedron
        { 5, 1, 4, 7 }, // 3rd corner tetrahedron
        { 6, 2, 4, 7 }, // 4th corner tetrahedron
        { 1, 2, 4, 7 }  // Central tetrahedron
    };

    static const unsigned neighbors[numTetrahedrons][4] =
    {
        { 4, M_MAX_UNSIGNED, M_MAX_UNSIGNED, M_MAX_UNSIGNED },
        { 4, M_MAX_UNSIGNED, M_MAX_UNSIGNED, M_MAX_UNSIGNED },
        { 4, M_MAX_UNSIGNED, M_MAX_UNSIGNED, M_MAX_UNSIGNED },
        { 4, M_MAX_UNSIGNED, M_MAX_UNSIGNED, M_MAX_UNSIGNED },
        { 3, 2, 1, 0 }, // Tetrahedrons with corners at (6, 5, 3, 0)
    };

    vertices_.resize(NumSuperMeshVertices);
    for (unsigned i = 0; i < NumSuperMeshVertices; ++i)
        vertices_[i] = boundingBox.min_ + boundingBox.Size() * offsets[i];

    tetrahedrons_.resize(numTetrahedrons);
    for (unsigned i = 0; i < numTetrahedrons; ++i)
    {
        Tetrahedron tetrahedron;
        for (unsigned j = 0; j < 4; ++j)
        {
            tetrahedron.indices_[j] = indices[i][j];
            tetrahedron.neighbors_[j] = neighbors[i][j];
        }
        tetrahedrons_[i] = tetrahedron;
    }
}

void TetrahedralMesh::BuildTetrahedrons(ea::span<const Vector3> positions)
{
    // Initialize context
    DelaunayContext ctx;
    ctx.circumspheres_.resize(tetrahedrons_.size());
    ctx.removed_.resize(tetrahedrons_.size());
    for (unsigned i = 0; i < tetrahedrons_.size(); ++i)
        ctx.circumspheres_[i] = GetTetrahedronCircumsphere(i);

    // Append vertices and initialize queue
    const unsigned startVertex = vertices_.size();
    vertices_.insert(vertices_.end(), positions.begin(), positions.end());

    ea::vector<unsigned> verticesQueue;
    for (unsigned newVertexIndex = startVertex; newVertexIndex < vertices_.size(); ++newVertexIndex)
        verticesQueue.push_back(newVertexIndex);

    // Triangulate
    TetrahedralMeshSurface holeSurface;
    ea::vector<unsigned> removedTetrahedrons;
    ea::vector<unsigned> postponedVertices;
    while (!verticesQueue.empty())
    {
        // Process current bunch of vertices
        postponedVertices.clear();
        for (unsigned newVertexIndex : verticesQueue)
        {
            // Carve hole in the mesh
            if (!FindAndRemoveIntersected(ctx, vertices_[newVertexIndex], holeSurface, removedTetrahedrons))
            {
                postponedVertices.push_back(newVertexIndex);
                continue;
            }

            // Disconnect carved out tetrahedrons
            DisconnectRemovedTetrahedrons(removedTetrahedrons);

            // Allocate space for new tetrahedrons
            while (removedTetrahedrons.size() < holeSurface.Size())
            {
                removedTetrahedrons.push_back(tetrahedrons_.size());
                tetrahedrons_.push_back();
                ctx.circumspheres_.push_back();
                ctx.removed_.push_back(true);
            }

            // Fill hole with tetrahedrons
            FillStarShapedHole(ctx, removedTetrahedrons, holeSurface, newVertexIndex);
        }

        // Re-enqueue postponed vertices
        ea::swap(postponedVertices, verticesQueue);

        // If all the vertices are postponed again, ignore them
        if (postponedVertices.size() == verticesQueue.size())
        {
            URHO3D_LOGWARNING("{} vertices are excluded from triangulation due to mathematical fluctuations",
                postponedVertices.size());
            break;
        }
    }

    // Dump failed attempts for debugging
    debugHighlightEdges_.clear();
    for (unsigned ignoredVertex : verticesQueue)
    {
        const Vector3 position = vertices_[verticesQueue[0]];
        FindAndRemoveIntersected(ctx, position, holeSurface, removedTetrahedrons, true);
    }

    // Finalize triangulation
    DisconnectSuperMeshTetrahedrons(ctx.removed_);
    RemoveMarkedTetrahedrons(ctx.removed_);
    RemoveSuperMeshVertices();
    UpdateIgnoredVertices();
    assert(IsAdjacencyValid(false));

    // Build the outer space and precompute matrices
    TetrahedralMeshSurface hullSurface;
    BuildHullSurface(hullSurface);

    CalculateHullNormals(hullSurface);
    BuildOuterTetrahedrons(hullSurface);
    CalculateInnerMatrices();
    CalculateOuterMatrices();
}

bool TetrahedralMesh::IsAdjacencyValid(bool fullyConnected) const
{
    for (unsigned tetIndex = 0; tetIndex < tetrahedrons_.size(); ++tetIndex)
    {
        for (unsigned faceIndex = 0; faceIndex < 4; ++faceIndex)
        {
            const unsigned neighborIndex = tetrahedrons_[tetIndex].neighbors_[faceIndex];
            if (neighborIndex != M_MAX_UNSIGNED)
            {
                const Tetrahedron& neighborTetrahedron = tetrahedrons_[neighborIndex];
                if (!neighborTetrahedron.HasNeighbor(tetIndex))
                    return false;
            }
            else if (fullyConnected)
                return false;
        }
    }
    return true;
}

bool TetrahedralMesh::FindAndRemoveIntersected(TetrahedralMesh::DelaunayContext& ctx, const Vector3& position,
    TetrahedralMeshSurface& holeSurface, ea::vector<unsigned>& removedTetrahedrons, bool dumpErrors) const
{
    // Reset output
    holeSurface.Clear();
    removedTetrahedrons.clear();

    // Find first tetrahedron to remove
    for (unsigned tetIndex = 0; tetIndex < tetrahedrons_.size(); ++tetIndex)
    {
        if (!ctx.removed_[tetIndex] && ctx.IsInsideCircumsphere(tetIndex, position))
        {
            removedTetrahedrons.push_back(tetIndex);
            ctx.removed_[tetIndex] = true;
            break;
        }
    }

    if (removedTetrahedrons.empty())
    {
        URHO3D_LOGERROR("Cannot update tetrahedral mesh for vertex at {}", position.ToString());
        assert(0);
        return false;
    }

    // Do breadth search to collect all bad tetrahedrons
    // Note: size may change during the loop
    for (unsigned i = 0; i < removedTetrahedrons.size(); ++i)
    {
        const unsigned tetIndex = removedTetrahedrons[i];
        const Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];

        // Process neighbors
        for (unsigned faceIndex = 0; faceIndex < 4; ++faceIndex)
        {
            const unsigned neighborTetIndex = tetrahedron.neighbors_[faceIndex];

            // Outer surface is reached
            if (neighborTetIndex == M_MAX_UNSIGNED)
                continue;

            // Ignore already removed tetrahedrons
            if (ctx.removed_[neighborTetIndex])
                continue;

            // If circumsphere of neighbor tetrahedron contains the point, remove this neighbor and queue it.
            if (ctx.IsInsideCircumsphere(neighborTetIndex, position))
            {
                removedTetrahedrons.push_back(neighborTetIndex);
                ctx.removed_[neighborTetIndex] = true;
            }
        }
    }

    // Collect triangles of the hole surface
    ctx.holeTriangles_.clear();
    for (unsigned tetIndex : removedTetrahedrons)
    {
        const Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];
        for (unsigned faceIndex = 0; faceIndex < 4; ++faceIndex)
        {
            const unsigned neighborTetIndex = tetrahedron.neighbors_[faceIndex];

            // Outer surface is reached
            if (neighborTetIndex == M_MAX_UNSIGNED)
            {
                // Face of outer surface doesn't have underlying tetrahedron
                const TetrahedralMeshSurfaceTriangle holeTriangle = tetrahedron.GetTriangleFace(
                    faceIndex, M_MAX_UNSIGNED, M_MAX_UNSIGNED);
                ctx.holeTriangles_.push_back(holeTriangle);
                continue;
            }

            // If neighbor is not removed, add face
            if (!ctx.removed_[neighborTetIndex])
            {
                const Tetrahedron& neighborTetrahedron = tetrahedrons_[neighborTetIndex];
                const unsigned neighborFaceIndex = neighborTetrahedron.GetNeighborFaceIndex(tetIndex);

                const TetrahedralMeshSurfaceTriangle holeTriangle = neighborTetrahedron.GetTriangleFace(
                    neighborFaceIndex, neighborTetIndex, neighborFaceIndex);
                ctx.holeTriangles_.push_back(holeTriangle);
            }
        }
    }

    // Verify that all hole triangles are faced in right direction
    bool valid = true;
    const HighPrecisionVector3 p0{ position };
    for (TetrahedralMeshSurfaceTriangle& triangle : ctx.holeTriangles_)
    {
        // Outer triangles are always oriented right
        if (triangle.tetIndex_ == M_MAX_UNSIGNED)
            continue;

        // Normalize triangle orientation
        triangle.Normalize(vertices_);

        const HighPrecisionVector3 p1{ vertices_[triangle.indices_[0]] };
        const HighPrecisionVector3 p2{ vertices_[triangle.indices_[1]] };
        const HighPrecisionVector3 p3{ vertices_[triangle.indices_[2]] };
        const HighPrecisionVector3 normal = (p2 - p1).CrossProduct(p3 - p1);
        const double distance = (p0 - p1).DotProduct(normal);

        // If coplanar or worse, cannot add new vertex
        if (distance < M_LARGE_EPSILON)
        {
            valid = false;
            break;
        }
    }

    // Revert all changes if invalid or if dump error mode is on
    if (!valid || dumpErrors)
    {
        if (dumpErrors)
        {
            assert(!valid);
            for (const TetrahedralMeshSurfaceTriangle& triangle : ctx.holeTriangles_)
            {
                const unsigned i0 = triangle.indices_[0];
                const unsigned i1 = triangle.indices_[1];
                const unsigned i2 = triangle.indices_[2];
                debugHighlightEdges_.emplace_back(i0, i1);
                debugHighlightEdges_.emplace_back(i1, i2);
                debugHighlightEdges_.emplace_back(i2, i0);
            }
        }

        for (unsigned tetIndex : removedTetrahedrons)
            ctx.removed_[tetIndex] = false;

        removedTetrahedrons.clear();
        return false;
    }

    // Build hole surface
    for (const TetrahedralMeshSurfaceTriangle& triangle : ctx.holeTriangles_)
    {
        if (!holeSurface.AddFace(triangle))
        {
            URHO3D_LOGERROR("Cannot update surface of the carved hole in tetrahedral mesh for vertex at {}",
                position.ToString());
            assert(0);
            holeSurface.Clear();
            return false;
        }
    }

    if (!holeSurface.IsClosedSurface())
    {
        URHO3D_LOGERROR("Surface of the carved hole in tetrahedral mesh is incomplete for vertex at {}",
            position.ToString());
        assert(0);
        holeSurface.Clear();
        return false;
    }

    return true;
}

void TetrahedralMesh::DisconnectRemovedTetrahedrons(const ea::vector<unsigned>& removedTetrahedrons)
{
    // Disconnect removed tetrahedrons
    for (unsigned tetIndex : removedTetrahedrons)
    {
        Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];
        for (unsigned faceIndex = 0; faceIndex < 4; ++faceIndex)
        {
            const unsigned neighborTetIndex = tetrahedron.neighbors_[faceIndex];
            if (neighborTetIndex != M_MAX_UNSIGNED)
            {
                Tetrahedron& neighborTetrahedron = tetrahedrons_[neighborTetIndex];
                const unsigned neighborFaceIndex = neighborTetrahedron.GetNeighborFaceIndex(tetIndex);

                tetrahedron.neighbors_[faceIndex] = M_MAX_UNSIGNED;
                neighborTetrahedron.neighbors_[neighborFaceIndex] = M_MAX_UNSIGNED;
            }
        }
    }
}

void TetrahedralMesh::FillStarShapedHole(TetrahedralMesh::DelaunayContext& ctx,
    const ea::vector<unsigned>& outputTetrahedrons, const TetrahedralMeshSurface& holeSurface, unsigned centerIndex)
{
    for (unsigned i = 0; i < holeSurface.Size(); ++i)
    {
        const unsigned newTetIndex = outputTetrahedrons[i];
        Tetrahedron& tetrahedron = tetrahedrons_[newTetIndex];
        const TetrahedralMeshSurfaceTriangle& holeTriangle = holeSurface.faces_[i];

        // Connect to newly added (or to be added) adjacent tetrahedrons filling the hole
        for (unsigned j = 0; j < 3; ++j)
        {
            tetrahedron.indices_[j] = holeTriangle.indices_[j];
            tetrahedron.neighbors_[j] = outputTetrahedrons[holeTriangle.neighbors_[j]];
        }

        // Connect to tetrahedron outside the hole
        tetrahedron.indices_[3] = centerIndex;
        tetrahedron.neighbors_[3] = holeTriangle.tetIndex_;
        if (holeTriangle.tetIndex_ != M_MAX_UNSIGNED)
        {
            Tetrahedron& neighborTetrahedron = tetrahedrons_[holeTriangle.tetIndex_];
            assert(neighborTetrahedron.neighbors_[holeTriangle.tetFace_] == M_MAX_UNSIGNED);
            neighborTetrahedron.neighbors_[holeTriangle.tetFace_] = newTetIndex;
        }

        ctx.removed_[newTetIndex] = false;
        ctx.circumspheres_[newTetIndex] = GetTetrahedronCircumsphere(newTetIndex);
    }
}

void TetrahedralMesh::DisconnectSuperMeshTetrahedrons(ea::vector<bool>& removed)
{
    for (unsigned tetIndex = 0; tetIndex < tetrahedrons_.size(); ++tetIndex)
    {
        // Any tetrahedron containing super-vertex is to be removed
        Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];
        for (unsigned j = 0; j < 4; ++j)
        {
            if (tetrahedron.indices_[j] < NumSuperMeshVertices)
            {
                removed[tetIndex] = true;
                break;
            }
        }

        if (!removed[tetIndex])
            continue;

        // Disconnect adjacency
        for (unsigned faceIndex = 0; faceIndex < 4; ++faceIndex)
        {
            const unsigned neighborIndex = tetrahedron.neighbors_[faceIndex];
            if (neighborIndex != M_MAX_UNSIGNED)
            {
                Tetrahedron& neighborTetrahedron = tetrahedrons_[neighborIndex];
                const unsigned neighborFaceIndex = neighborTetrahedron.GetNeighborFaceIndex(tetIndex);
                assert(neighborFaceIndex < 4);
                neighborTetrahedron.neighbors_[neighborFaceIndex] = M_MAX_UNSIGNED;
            }
        }
    }
}

void TetrahedralMesh::RemoveMarkedTetrahedrons(const ea::vector<bool>& removed)
{
    // Prepare for reconstruction
    ea::vector<Tetrahedron> tetrahedronsCopy = ea::move(tetrahedrons_);
    ea::vector<unsigned> oldToNewIndexMap(tetrahedronsCopy.size());
    tetrahedrons_.clear();

    // Rebuild array and create index
    for (unsigned oldTetIndex = 0; oldTetIndex < tetrahedronsCopy.size(); ++oldTetIndex)
    {
        if (removed[oldTetIndex])
        {
            oldToNewIndexMap[oldTetIndex] = M_MAX_UNSIGNED;
            continue;
        }

        oldToNewIndexMap[oldTetIndex] = tetrahedrons_.size();
        tetrahedrons_.push_back(tetrahedronsCopy[oldTetIndex]);
    }

    // Adjust neighbor indices
    for (Tetrahedron& tetrahedron : tetrahedrons_)
    {
        for (unsigned faceIndex = 0; faceIndex < 4; ++faceIndex)
        {
            const unsigned oldIndex = tetrahedron.neighbors_[faceIndex];
            if (oldIndex != M_MAX_UNSIGNED)
            {
                const unsigned newIndex = oldToNewIndexMap[oldIndex];
                assert(newIndex != M_MAX_UNSIGNED);
                tetrahedron.neighbors_[faceIndex] = newIndex;
            }
        }
    }
}

void TetrahedralMesh::RemoveSuperMeshVertices()
{
    ea::rotate(vertices_.begin(), vertices_.begin() + NumSuperMeshVertices, vertices_.end());

    for (unsigned tetIndex = 0; tetIndex < tetrahedrons_.size(); ++tetIndex)
    {
        for (unsigned faceIndex = 0; faceIndex < 4; ++faceIndex)
            tetrahedrons_[tetIndex].indices_[faceIndex] -= NumSuperMeshVertices;
    }

    for (auto& edge : debugHighlightEdges_)
    {
        for (unsigned* index : { &edge.first, &edge.second })
        {
            if (*index < NumSuperMeshVertices)
                *index += vertices_.size() - NumSuperMeshVertices;
            else
                *index -= NumSuperMeshVertices;
        }
    }
}

void TetrahedralMesh::UpdateIgnoredVertices()
{
    ea::vector<bool> ignored_(vertices_.size(), true);
    for (const Tetrahedron& tetrahedron : tetrahedrons_)
    {
        for (unsigned faceIndex = 0; faceIndex < 4; ++faceIndex)
            ignored_[tetrahedron.indices_[faceIndex]] = false;
    }

    ignoredVertices_.clear();
    for (unsigned vertexIndex = 0; vertexIndex < vertices_.size() - NumSuperMeshVertices; ++vertexIndex)
    {
        if (ignored_[vertexIndex])
            ignoredVertices_.push_back(vertexIndex);
    }
}

void TetrahedralMesh::BuildHullSurface(TetrahedralMeshSurface& hullSurface)
{
    hullSurface.Clear();
    for (unsigned tetIndex = 0; tetIndex < tetrahedrons_.size(); ++tetIndex)
    {
        const Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];
        for (unsigned faceIndex = 0; faceIndex < 4; ++faceIndex)
        {
            if (tetrahedron.neighbors_[faceIndex] != M_MAX_UNSIGNED)
                continue;

            const TetrahedralMeshSurfaceTriangle& hullTriangle = tetrahedron.GetTriangleFace(
                faceIndex, tetIndex, faceIndex);
            hullSurface.AddFace(hullTriangle);
        }
    }

    for (TetrahedralMeshSurfaceTriangle& hullTriangle : hullSurface.faces_)
        hullTriangle.Normalize(vertices_);

    assert(hullSurface.IsClosedSurface());
}

void TetrahedralMesh::CalculateHullNormals(const TetrahedralMeshSurface& hullSurface)
{
    hullNormals_.resize(vertices_.size());

    for (const TetrahedralMeshSurfaceTriangle& triangle : hullSurface.faces_)
    {
        const Vector3 p1 = vertices_[triangle.indices_[0]];
        const Vector3 p2 = vertices_[triangle.indices_[1]];
        const Vector3 p3 = vertices_[triangle.indices_[2]];
        const Vector3 normal = (p2 - p1).CrossProduct(p3 - p1);

        // Accumulate vertex normals
        for (unsigned j = 0; j < 3; ++j)
            hullNormals_[triangle.indices_[j]] += normal;
    }

    // Normalize outputs
    for (Vector3& normal : hullNormals_)
    {
        if (normal != Vector3::ZERO)
            normal.Normalize();
    }
}

void TetrahedralMesh::BuildOuterTetrahedrons(const TetrahedralMeshSurface& hullSurface)
{
    numInnerTetrahedrons_ = tetrahedrons_.size();
    tetrahedrons_.resize(numInnerTetrahedrons_ + hullSurface.Size());

    for (unsigned tetIndex = numInnerTetrahedrons_; tetIndex < tetrahedrons_.size(); ++tetIndex)
    {
        Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];
        const TetrahedralMeshSurfaceTriangle& hullTrinagle = hullSurface.faces_[tetIndex - numInnerTetrahedrons_];

        for (unsigned faceIndex = 0; faceIndex < 3; ++faceIndex)
        {
            tetrahedron.indices_[faceIndex] = hullTrinagle.indices_[faceIndex];
            tetrahedron.neighbors_[faceIndex] = numInnerTetrahedrons_ + hullTrinagle.neighbors_[faceIndex];
        }

        tetrahedron.indices_[3] = Tetrahedron::Infinity3;
        tetrahedron.neighbors_[3] = hullTrinagle.tetIndex_;
        tetrahedrons_[hullTrinagle.tetIndex_].neighbors_[hullTrinagle.tetFace_] = tetIndex;
    }

    assert(IsAdjacencyValid(true));
}

void TetrahedralMesh::CalculateInnerMatrices()
{
    for (unsigned tetIndex = 0; tetIndex < numInnerTetrahedrons_; ++tetIndex)
    {
        Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];
        const Vector3 p0 = vertices_[tetrahedron.indices_[0]];
        const Vector3 p1 = vertices_[tetrahedron.indices_[1]];
        const Vector3 p2 = vertices_[tetrahedron.indices_[2]];
        const Vector3 p3 = vertices_[tetrahedron.indices_[3]];
        const Vector3 u1 = p1 - p0;
        const Vector3 u2 = p2 - p0;
        const Vector3 u3 = p3 - p0;
        tetrahedron.matrix_ = Matrix3(u1.x_, u2.x_, u3.x_, u1.y_, u2.y_, u3.y_, u1.z_, u2.z_, u3.z_).Inverse();
    }
}

void TetrahedralMesh::CalculateOuterMatrices()
{
    for (unsigned tetIndex = numInnerTetrahedrons_; tetIndex < tetrahedrons_.size(); ++tetIndex)
    {
        Tetrahedron& tetrahedron = tetrahedrons_[tetIndex];

        Vector3 positions[3];
        Vector3 normals[3];
        for (unsigned i = 0; i < 3; ++i)
        {
            positions[i] = vertices_[tetrahedron.indices_[i]];
            normals[i] = hullNormals_[tetrahedron.indices_[i]];
        }

        const Vector3 A = positions[0] - positions[2];
        const Vector3 Ap = normals[0] - normals[2];
        const Vector3 B = positions[1] - positions[2];
        const Vector3 Bp = normals[1] - normals[2];
        const Vector3 P2 = positions[2];
        const Vector3 Cp = -normals[2];

        Matrix3x4& m = tetrahedron.matrix_;

        m.m00_ = // input.x *
            + Ap.y_ * Bp.z_
            - Ap.z_ * Bp.y_;
        m.m01_ = // input.y *
            - Ap.x_ * Bp.z_
            + Ap.z_ * Bp.x_;
        m.m02_ = // input.z *
            + Ap.x_ * Bp.y_
            - Ap.y_ * Bp.x_;
        m.m03_ = // 1 *
            + A.x_ * Bp.y_* Cp.z_
            - A.y_ * Bp.x_ * Cp.z_
            + Ap.x_ * B.y_ * Cp.z_
            - Ap.y_ * B.x_ * Cp.z_
            + A.z_ * Bp.x_ * Cp.y_
            - A.z_ * Bp.y_ * Cp.x_
            + Ap.z_ * B.x_ * Cp.y_
            - Ap.z_ * B.y_ * Cp.x_
            - A.x_ * Bp.z_ * Cp.y_
            + A.y_ * Bp.z_ * Cp.x_
            - Ap.x_ * B.z_ * Cp.y_
            + Ap.y_ * B.z_ * Cp.x_;
        m.m03_ -= P2.x_ * m.m00_ + P2.y_ * m.m01_ + P2.z_ * m.m02_;

        m.m10_ = // input.x *
            + Ap.y_ * B.z_
            + A.y_ * Bp.z_
            - Ap.z_ * B.y_
            - A.z_ * Bp.y_;
        m.m11_ = // input.y *
            - A.x_ * Bp.z_
            - Ap.x_ * B.z_
            + A.z_ * Bp.x_
            + Ap.z_ * B.x_;
        m.m12_ = // input.z *
            + A.x_ * Bp.y_
            - A.y_ * Bp.x_
            + Ap.x_ * B.y_
            - Ap.y_ * B.x_;
        m.m13_ = // 1 *
            + A.x_ * B.y_ * Cp.z_
            - A.y_ * B.x_ * Cp.z_
            - A.x_ * B.z_ * Cp.y_
            + A.y_ * B.z_ * Cp.x_
            + A.z_ * B.x_ * Cp.y_
            - A.z_ * B.y_ * Cp.x_;
        m.m13_ -= P2.x_ * m.m10_ + P2.y_ * m.m11_ + P2.z_ * m.m12_;

        m.m20_ = // input.x *
            - A.z_ * B.y_
            + A.y_ * B.z_;
        m.m21_ = // input.y *
            - A.x_ * B.z_
            + A.z_ * B.x_;
        m.m22_ = // input.z *
            + A.x_ * B.y_
            - A.y_ * B.x_;
        m.m23_ = 0.0f; // 1 *
        m.m23_ -= P2.x_ * m.m20_ + P2.y_ * m.m21_ + P2.z_ * m.m22_;

        const float a =
            + Ap.x_ * Bp.y_ * Cp.z_
            - Ap.y_ * Bp.x_ * Cp.z_
            + Ap.z_ * Bp.x_ * Cp.y_
            - Ap.z_ * Bp.y_ * Cp.x_
            + Ap.y_ * Bp.z_ * Cp.x_
            - Ap.x_ * Bp.z_ * Cp.y_;

        if (Abs(a) > M_EPSILON)
        {
            // d is not zero, so the polynomial at^3 + bt^2 + ct + d = 0 is actually cubic
            // and we can simplify to the monic form t^3 + pt^2 + qt + r = 0
            m = m * (1.0f / a);
        }
        else
        {
            // It's actually a quadratic or even linear equation
            tetrahedron.indices_[3] = Tetrahedron::Infinity2;
        }
    }
}

bool SerializeValue(Archive& archive, const char* name, Tetrahedron& value)
{
    if (ArchiveBlock block = archive.OpenUnorderedBlock(name))
    {
        SerializeValue(archive, "Index0", value.indices_[0]);
        SerializeValue(archive, "Index1", value.indices_[1]);
        SerializeValue(archive, "Index2", value.indices_[2]);
        SerializeValue(archive, "Index3", value.indices_[3]);
        SerializeValue(archive, "Neighbor0", value.neighbors_[0]);
        SerializeValue(archive, "Neighbor1", value.neighbors_[1]);
        SerializeValue(archive, "Neighbor2", value.neighbors_[2]);
        SerializeValue(archive, "Neighbor3", value.neighbors_[3]);
        SerializeValue(archive, "Matrix", value.matrix_);
        return true;
    }
    return false;
}

bool SerializeValue(Archive& archive, const char* name, TetrahedralMesh& value)
{
    if (ArchiveBlock block = archive.OpenUnorderedBlock(name))
    {
        SerializeVector(archive, "Vertices", "Position", value.vertices_);
        SerializeVector(archive, "Tetrahedrons", "Tetrahedron", value.tetrahedrons_);
        SerializeVector(archive, "HullNormals", "Hulls", value.hullNormals_);
        SerializeValue(archive, "NumInnerTetrahedrons", value.numInnerTetrahedrons_);
        return true;
    }
    return false;
}

}
