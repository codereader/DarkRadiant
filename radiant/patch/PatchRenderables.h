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
    const PatchTesselation& m_tess;
    mutable render::VertexBuffer _vertexBuf;

public:

    RenderablePatchWireframe(const PatchTesselation& tess) : m_tess(tess)
    { }

    void render(const RenderInfo& info) const;
};

/// Helper class to render a fixed geometry PatchTesselation in wireframe mode
class RenderablePatchFixedWireframe : public OpenGLRenderable
{
    PatchTesselation& m_tess;
    mutable render::IndexedVertexBuffer _vertexBuf;

public:

    RenderablePatchFixedWireframe(PatchTesselation& tess) : m_tess(tess)
    { }

    void render(const RenderInfo& info) const;
};

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

//#define PATCHES_USE_VBO

/// Helper class to render a PatchTesselation in solid mode
class RenderablePatchSolid :
	public OpenGLRenderable
{
	PatchTesselation& m_tess;

	// Vertex buffer objects
	GLuint _vboData;
	GLuint _vboIdx;

public:
	RenderablePatchSolid(PatchTesselation& tess);

	// Updates rendering structures
	void update();

	// Implementation is in Patch.cpp
	void RenderNormals() const;

	void render(const RenderInfo& info) const;
};
