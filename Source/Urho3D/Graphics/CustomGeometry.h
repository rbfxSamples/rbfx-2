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

#pragma once

#include "../Graphics/Drawable.h"

namespace Urho3D
{

/// Custom geometry vertex.
struct CustomGeometryVertex
{
    /// Position.
    Vector3 position_;
    /// Normal.
    Vector3 normal_;
    /// Color.
    unsigned color_;
    /// Texture coordinates.
    Vector2 texCoord_;
    /// Tangent.
    Vector4 tangent_;
};

class VertexBuffer;

/// Custom geometry component.
class URHO3D_API CustomGeometry : public Drawable
{
    URHO3D_OBJECT(CustomGeometry, Drawable);

public:
    /// Construct.
    explicit CustomGeometry(Context* context);
    /// Destruct.
    ~CustomGeometry() override;
    /// Register object factory. Drawable must be registered first.
    static void RegisterObject(Context* context);

    /// Process octree raycast. May be called from a worker thread.
    void ProcessRayQuery(const RayOctreeQuery& query, PODVector<RayQueryResult>& results) override;
    /// Return the geometry for a specific LOD level.
    Geometry* GetLodGeometry(unsigned batchIndex, unsigned level) override;
    /// Return number of occlusion geometry triangles.
    unsigned GetNumOccluderTriangles() override;
    /// Draw to occlusion buffer. Return true if did not run out of triangles.
    bool DrawOcclusion(OcclusionBuffer* buffer) override;

    /// Geometry Shape Helper Functions (normal defaults to (0,1,0)

    /// Shape Creation Functions, can be used as input into protrude shape
    static Vector<Vector3> GetCircleShape(float radius = 1, size_t iterations = 100, float startTheta = 0, float endTheta = 2 * M_PI);
    static Vector<Vector3> GetSquareShape(float size);

    /// Make the custom geometry into a circle, change start and stop to make a segment instead
    /// Set clear=false and geomNum for making multiple circle segments
    void MakeCircle(float radius = 1, size_t iterations = 300, float startTheta = 0, float endTheta = 2 * M_PI,
        bool clear = true, int geomNum = 0);
    /// Draws a shape generated by connecting the points in the input vector (the end point will connect to the start)
    void MakeShape(const Vector<Urho3D::Vector3>& pointList, bool connectTail = true);
    /// Makes this custom geometry into a square shape
    void MakeSquare(float size);
    /// Produces a circle graph given a vector of paired (weights and materials)
    void MakeCircleGraph(const Vector<Pair<float, Urho3D::SharedPtr<Urho3D::Material> > >& parts, 
        int radius = 1, int iterations = 300);
    void MakeSphere(float radius = 1, size_t iterations = 200);
    /// Protrode a shape along a line (note: the input vector must be a complete shape with the tail connecting to head)
    /// This function makes this object into the newly generated 3D mesh (works best if the line (point list) is also complete)
    void ProtrudeShape(const Vector<Vector3> &mShapeList, const Vector<Vector3> &mPointList, bool connectTail = false);

    /// Helper Function Creating 3D Meshes (connectTail = true), connect the endLinePoint to the start)
    void CreateQuadsFromBuffer(const Vector<Vector3>& pointList, size_t zIterations, size_t thetaIterations, bool connectTail = false);
    /// Fills a point List shape with triangles
    void FillShape(const Vector<Vector3>& shapeList, bool connectTail = true, bool clear = true, int geomNum = 0);
    /// End Shape Generation Helper Functions

    /// Clear all geometries.
    void Clear();
    /// Set number of geometries.
    void SetNumGeometries(unsigned num);
    /// Set vertex buffer dynamic mode. A dynamic buffer should be faster to update frequently. Effective at the next Commit() call.
    void SetDynamic(bool enable);
    /// Begin defining a geometry. Clears existing vertices in that index.
    void BeginGeometry(unsigned index, PrimitiveType type);
    /// Define a vertex position. This begins a new vertex.
    void DefineVertex(const Vector3& position);
    /// Define a vertex normal.
    void DefineNormal(const Vector3& normal);
    /// Define a vertex color.
    void DefineColor(const Color& color);
    /// Define a vertex UV coordinate.
    void DefineTexCoord(const Vector2& texCoord);
    /// Define a vertex tangent.
    void DefineTangent(const Vector4& tangent);
    /// Set the primitive type, number of vertices and elements in a geometry, after which the vertices can be edited with GetVertex(). An alternative to BeginGeometry() / DefineVertex().
    void DefineGeometry
        (unsigned index, PrimitiveType type, unsigned numVertices, bool hasNormals, bool hasColors, bool hasTexCoords,
            bool hasTangents);
    /// Update vertex buffer and calculate the bounding box. Call after finishing defining geometry.
    void Commit();
    /// Set material on all geometries.
    void SetMaterial(Material* material);
    /// Set material on one geometry. Return true if successful.
    bool SetMaterial(unsigned index, Material* material);

    /// Return number of geometries.
    unsigned GetNumGeometries() const { return geometries_.Size(); }

    /// Return number of vertices in a geometry.
    unsigned GetNumVertices(unsigned index) const;

    /// Return whether vertex buffer dynamic mode is enabled.
    bool IsDynamic() const { return dynamic_; }

    /// Return material by geometry index.
    Material* GetMaterial(unsigned index = 0) const;

    /// Return all vertices. These can be edited; calling Commit() updates the vertex buffer.
    Vector<PODVector<CustomGeometryVertex> >& GetVertices() { return vertices_; }

    /// Return a vertex in a geometry for editing, or null if out of bounds. After the edits are finished, calling Commit() updates  the vertex buffer.
    CustomGeometryVertex* GetVertex(unsigned geometryIndex, unsigned vertexNum);

    /// Set geometry data attribute.
    void SetGeometryDataAttr(const PODVector<unsigned char>& value);
    /// Set materials attribute.
    void SetMaterialsAttr(const ResourceRefList& value);
    /// Return geometry data attribute.
    PODVector<unsigned char> GetGeometryDataAttr() const;
    /// Return materials attribute.
    const ResourceRefList& GetMaterialsAttr() const;

protected:
    /// Recalculate the world-space bounding box.
    void OnWorldBoundingBoxUpdate() override;

private:
    /// Primitive type per geometry.
    PODVector<PrimitiveType> primitiveTypes_;
    /// Source vertices per geometry.
    Vector<PODVector<CustomGeometryVertex> > vertices_;
    /// All geometries.
    Vector<SharedPtr<Geometry> > geometries_;
    /// Vertex buffer.
    SharedPtr<VertexBuffer> vertexBuffer_;
    /// Element mask used so far.
    VertexMaskFlags elementMask_;
    /// Current geometry being updated.
    unsigned geometryIndex_;
    /// Material list attribute.
    mutable ResourceRefList materialsAttr_;
    /// Vertex buffer dynamic flag.
    bool dynamic_;
};

}
