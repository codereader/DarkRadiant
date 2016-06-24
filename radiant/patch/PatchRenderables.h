/**
 * \file
 * These are the renderables that are used in the PatchNode/Patch class to draw
 * the patch onto the screen.
 */
#pragma once

#include "igl.h"
#include "PatchTesselation.h"

#include "render/VertexBuffer.h"
#include "render/IndexedVertexBuffer.h"

/// Helper class to render a PatchTesselation in wireframe mode
class RenderablePatchWireframe :
	public OpenGLRenderable
{
protected:
	// Geometry source
	const PatchTesselation& _tess;

	// VertexBuffer for rendering
	typedef render::IndexedVertexBuffer<Vertex3f> VertexBuffer_T;
	mutable VertexBuffer_T _vertexBuf;

	mutable bool _needsUpdate;

public:
	RenderablePatchWireframe(const PatchTesselation& tess) :
		_tess(tess),
		_needsUpdate(true)
	{ }

	virtual ~RenderablePatchWireframe() {}

    void render(const RenderInfo& info) const;

    void queueUpdate();
};

/// Helper class to render a fixed geometry PatchTesselation in wireframe mode
class RenderablePatchFixedWireframe : 
	public RenderablePatchWireframe
{
public:
    RenderablePatchFixedWireframe(PatchTesselation& tess) : 
		RenderablePatchWireframe(tess)
    {}
};

/// Helper class to render a PatchTesselation in solid mode
class RenderablePatchSolid :
	public OpenGLRenderable
{
    // Geometry source
	PatchTesselation& _tess;

    // VertexBuffer for rendering
    typedef render::IndexedVertexBuffer<ArbitraryMeshVertex> VertexBuffer_T;
    mutable VertexBuffer_T _vertexBuf;

    mutable bool _needsUpdate;

public:
	RenderablePatchSolid(PatchTesselation& tess);

	void render(const RenderInfo& info) const;

    void queueUpdate();
};

// Renders a vertex' normal/tangent/bitangent vector (for debugging purposes)
class RenderablePatchVectorsNTB :
	public OpenGLRenderable
{
private:
    std::vector<VertexCb> _vertices;
	const PatchTesselation& _tess;

	ShaderPtr _shader;

public:
	const ShaderPtr& getShader() const;

	RenderablePatchVectorsNTB(const PatchTesselation& tess);

	void setRenderSystem(const RenderSystemPtr& renderSystem);

	void render(const RenderInfo& info) const;

	void render(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const;
};
