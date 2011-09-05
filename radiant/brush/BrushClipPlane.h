#pragma once

#include "math/Plane3.h"
#include "irender.h"
#include "irenderable.h"
#include "Winding.h"

class BrushClipPlane : 
	public OpenGLRenderable
{
private:
	Plane3 m_plane;
	Winding m_winding;
	ShaderPtr m_state;

public:
    virtual ~BrushClipPlane() {}

	void setPlane(const Brush& brush, const Plane3& plane) {
		m_plane = plane;
		if (m_plane.isValid()) {
			brush.windingForClipPlane(m_winding, m_plane);
		}
		else {
			m_winding.resize(0);
		}
		m_winding.updateNormals(m_plane.normal());
	}

	void render(const RenderInfo& info) const {
		if (info.checkFlag(RENDER_FILL)) {
			m_winding.render(info);
		}
		else {
			m_winding.drawWireframe();
		}
	}

	void setRenderSystem(const RenderSystemPtr& renderSystem)
	{
		if (renderSystem)
		{
			m_state = renderSystem->capture("$CLIPPER_OVERLAY");
		}
		else
		{
			m_state.reset();
		}
	}

	void render(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const
	{
		collector.SetState(m_state, RenderableCollector::eWireframeOnly);
		collector.SetState(m_state, RenderableCollector::eFullMaterials);
		collector.addRenderable(*this, localToWorld);
	}
};
