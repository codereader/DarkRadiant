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
class RenderablePatchWireframe : public OpenGLRenderable
{
    // Geometry source
    const PatchTesselation& _tess;

    // VertexBuffer for rendering
    typedef render::VertexBuffer<Vertex3f> VertexBuffer_T;
    mutable VertexBuffer_T _vertexBuf;

    mutable bool _needsUpdate;

public:

    RenderablePatchWireframe(const PatchTesselation& tess) : 
        _tess(tess),
        _needsUpdate(true)
    { }

    void render(const RenderInfo& info) const;

    void queueUpdate();
};

/// Helper class to render a fixed geometry PatchTesselation in wireframe mode
class RenderablePatchFixedWireframe : public OpenGLRenderable
{
    // Geometry source
    PatchTesselation& _tess;

    // VertexBuffer for rendering
    typedef render::IndexedVertexBuffer<Vertex3f> VertexBuffer_T;
    mutable VertexBuffer_T _vertexBuf;

    mutable bool _needsUpdate;

public:

    RenderablePatchFixedWireframe(PatchTesselation& tess) : 
        _tess(tess),
        _needsUpdate(true)
    {}

    void render(const RenderInfo& info) const;

    void queueUpdate();
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
