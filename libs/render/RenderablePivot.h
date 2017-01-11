#pragma once

#include "irender.h"
#include "irenderable.h"
#include "igl.h"

#include <vector>
#include "math/Vector3.h"
#include "math/Matrix4.h"

#include "Colour4b.h"
#include "VertexCb.h"

namespace render
{

class RenderablePivot :
	public OpenGLRenderable
{
private:
	const Colour4b _colourX;
	const Colour4b _colourY;
	const Colour4b _colourZ;

	std::vector<VertexCb> _vertices;
	const Vector3& _pivot;

	ShaderPtr _shader;

public:
	mutable Matrix4 m_localToWorld;

	const ShaderPtr& getShader() const
	{
		return _shader;
	}

	RenderablePivot(const Vector3& pivot) :
		_colourX(255, 0, 0, 255),
		_colourY(0, 255, 0, 255),
		_colourZ(0, 0, 255, 255),
		_pivot(pivot)
	{
		_vertices.reserve(6);

		_vertices.push_back(VertexCb(_pivot, _colourX));
		_vertices.push_back(VertexCb(_pivot + Vector3(16, 0, 0), _colourX));

		_vertices.push_back(VertexCb(_pivot, _colourY));
		_vertices.push_back(VertexCb(_pivot + Vector3(0, 16, 0), _colourY));

		_vertices.push_back(VertexCb(_pivot, _colourZ));
		_vertices.push_back(VertexCb(_pivot + Vector3(0, 0, 16), _colourZ));
	}

	/** greebo: Updates the renderable vertex array to the given pivot point
	*/
	void updatePivot()
	{
		_vertices.clear();

		_vertices.push_back(VertexCb(_pivot, _colourX));
		_vertices.push_back(VertexCb(_pivot + Vector3(16, 0, 0), _colourX));

		_vertices.push_back(VertexCb(_pivot, _colourY));
		_vertices.push_back(VertexCb(_pivot + Vector3(0, 16, 0), _colourY));

		_vertices.push_back(VertexCb(_pivot, _colourZ));
		_vertices.push_back(VertexCb(_pivot + Vector3(0, 0, 16), _colourZ));
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

	void render(const RenderInfo& info) const
	{
		if (_vertices.empty()) return;

		glEnableClientState(GL_COLOR_ARRAY);

		glVertexPointer(3, GL_DOUBLE, sizeof(VertexCb), &_vertices.data()->vertex);
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexCb), &_vertices.data()->colour);
		glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(_vertices.size()));
	}

	void render(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const
	{
		collector.PushState();

		// greebo: Commented this out to avoid the point from being moved along with the view.
		//Pivot2World_worldSpace(m_localToWorld, localToWorld, volume.GetModelview(), volume.GetProjection(), volume.GetViewport());

		collector.setHighlightFlag(RenderableCollector::Highlight::Primitives, false);
		collector.SetState(_shader, RenderableCollector::eWireframeOnly);
		collector.SetState(_shader, RenderableCollector::eFullMaterials);
		collector.addRenderable(*this, localToWorld);

		collector.PopState();
	}
};

}
