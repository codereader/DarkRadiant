#pragma once

#include "irender.h"
#include "irenderable.h"
#include "render/RenderableGeometry.h"

#include <vector>
#include "math/Vector3.h"
#include "math/Vector4.h"

namespace render
{

class RenderablePivot :
	public RenderableGeometry
{
private:
#if 0
	std::vector<VertexCb> _vertices;
#endif
	const Vector3& _pivot;
    bool _needsUpdate;

public:
    // Pass a reference to the pivot is in world coordinates
	RenderablePivot(const Vector3& pivot) :
		_pivot(pivot),
        _needsUpdate(true)
	{
#if 0
		_vertices.reserve(6);

		_vertices.push_back(VertexCb(_pivot, ColourX));
		_vertices.push_back(VertexCb(_pivot + Vector3(16, 0, 0), ColourX));

		_vertices.push_back(VertexCb(_pivot, ColourY));
		_vertices.push_back(VertexCb(_pivot + Vector3(0, 16, 0), ColourY));

		_vertices.push_back(VertexCb(_pivot, ColourZ));
		_vertices.push_back(VertexCb(_pivot + Vector3(0, 0, 16), ColourZ));
#endif
	}

    void queueUpdate()
    {
        _needsUpdate = true;
    }

#if 0
	/** greebo: Updates the renderable vertex array to the given pivot point
	*/
	void updatePivot()
	{
		_vertices.clear();

		_vertices.push_back(VertexCb(_pivot, ColourX));
		_vertices.push_back(VertexCb(_pivot + Vector3(16, 0, 0), ColourX));

		_vertices.push_back(VertexCb(_pivot, ColourY));
		_vertices.push_back(VertexCb(_pivot + Vector3(0, 16, 0), ColourY));

		_vertices.push_back(VertexCb(_pivot, ColourZ));
		_vertices.push_back(VertexCb(_pivot + Vector3(0, 0, 16), ColourZ));
	}

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
#endif

#if 0
	void render(const RenderInfo& info) const
	{
		if (_vertices.empty()) return;

		glEnableClientState(GL_COLOR_ARRAY);

		glVertexPointer(3, GL_DOUBLE, sizeof(VertexCb), &_vertices.data()->vertex);
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexCb), &_vertices.data()->colour);
		glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(_vertices.size()));

		glDisableClientState(GL_COLOR_ARRAY);
	}
#endif

#if 0
	void render(IRenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const
	{
		// greebo: Commented this out to avoid the point from being moved along with the view.
		//Pivot2World_worldSpace(m_localToWorld, localToWorld, volume.GetModelview(), volume.GetProjection(), volume.GetViewport());

		collector.addRenderable(*_shader, *this, localToWorld);
	}
#endif

protected:
    void updateGeometry() override
    {
        if (!_needsUpdate) return;

        _needsUpdate = false;

        static const Vector4 ColourX{ 255, 0, 0, 255 };
        static const Vector4 ColourY{ 0, 255, 0, 255 };
        static const Vector4 ColourZ{ 0, 0, 255, 255 };

        std::vector<ArbitraryMeshVertex> vertices;

        vertices.push_back(ArbitraryMeshVertex(_pivot, { 0, 0, 0 }, { 0, 0 }, ColourX));
        vertices.push_back(ArbitraryMeshVertex(_pivot + Vector3(16, 0, 0), { 0, 0, 0 }, { 0, 0 }, ColourX));

        vertices.push_back(ArbitraryMeshVertex(_pivot, { 0, 0, 0 }, { 0, 0 }, ColourY));
        vertices.push_back(ArbitraryMeshVertex(_pivot + Vector3(0, 16, 0), { 0, 0, 0 }, { 0, 0 }, ColourY));

        vertices.push_back(ArbitraryMeshVertex(_pivot, { 0, 0, 0 }, { 0, 0 }, ColourZ));
        vertices.push_back(ArbitraryMeshVertex(_pivot + Vector3(0, 0, 16), { 0, 0, 0 }, { 0, 0 }, ColourZ));

        static std::vector<unsigned int> Indices =
        {
            0, 1,
            2, 3,
            4, 5
        };

        RenderableGeometry::updateGeometry(GeometryType::Lines, vertices, Indices);
    }
};

}
