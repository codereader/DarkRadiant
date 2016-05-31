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
    typedef render::IndexedVertexBuffer<Vertex3f> VertexBuffer_T;
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

#define	VectorMA( v, s, b, o )		((o)[0]=(v)[0]+(b)[0]*(s),(o)[1]=(v)[1]+(b)[1]*(s),(o)[2]=(v)[2]+(b)[2]*(s))

class RenderablePatchVectors :
	public OpenGLRenderable
{
private:
    std::vector<VertexCb> _vertices;
	const PatchTesselation& _tess;

	ShaderPtr _shader;

public:
	const ShaderPtr& getShader() const
	{
		return _shader;
	}

	RenderablePatchVectors(const PatchTesselation& tess) :
		_tess(tess)
	{}

	void setRenderSystem(const RenderSystemPtr& renderSystem)
	{
		if (renderSystem)
		{
			_shader = renderSystem->capture("$PIVOT");
		}
		else
		{
			_shader.reset();
		}
	}

	void render(const RenderInfo& info) const
	{
		if (_tess.vertices.empty()) return;

        glBegin( GL_LINES );

        for (int j = 0; j < _tess.vertices.size(); j++)
        {
			const ArbitraryMeshVertex& v = _tess.vertices[j];

            Vector3 end;

			glColor3f( 0, 0, 1 );
			glVertex3dv(static_cast<double*>(Vector3(v.vertex)));
			VectorMA( v.vertex, 5, v.normal, end );
			glVertex3dv( static_cast<double*>(end) );

			glColor3f( 1, 0, 0 );
			glVertex3dv(static_cast<double*>(Vector3(v.vertex)));
			VectorMA( v.vertex, 5, v.tangent, end );
			glVertex3dv( static_cast<double*>(end) );

			glColor3f( 0, 1, 0 );
			glVertex3dv(static_cast<double*>(Vector3(v.vertex)));
			VectorMA( v.vertex, 5, v.bitangent, end );
			glVertex3dv( static_cast<double*>(end) );

			glColor3f( 1, 1, 1 );
			glVertex3dv(static_cast<double*>(Vector3(v.vertex)));
			glVertex3dv(static_cast<double*>(Vector3(v.vertex)));
		}

		glEnd();
	}

	void render(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const
	{
		collector.PushState();

		// greebo: Commented this out to avoid the point from being moved along with the view.
		//Pivot2World_worldSpace(m_localToWorld, localToWorld, volume.GetModelview(), volume.GetProjection(), volume.GetViewport());

		collector.highlightPrimitives(false);
		collector.SetState(_shader, RenderableCollector::eWireframeOnly);
		collector.SetState(_shader, RenderableCollector::eFullMaterials);
		collector.addRenderable(*this, localToWorld);

		collector.PopState();
	}
};
