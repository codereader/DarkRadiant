#ifndef BRUSHCLIPPLANE_H_
#define BRUSHCLIPPLANE_H_

#include "math/Plane3.h"
#include "irender.h"
#include "irenderable.h"
#include "Winding.h"

class BrushClipPlane : public OpenGLRenderable {
	Plane3 m_plane;
	Winding m_winding;
	static ShaderPtr m_state;
public:
    virtual ~BrushClipPlane() {}

	static void constructStatic() {
		m_state = GlobalRenderSystem().capture("$CLIPPER_OVERLAY");
	}
	static void destroyStatic() {
		m_state = ShaderPtr();
	}

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

	void render(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const {
		collector.SetState(m_state, RenderableCollector::eWireframeOnly);
		collector.SetState(m_state, RenderableCollector::eFullMaterials);
		collector.addRenderable(*this, localToWorld);
	}
}; // class BrushClipPlane

#endif /*BRUSHCLIPPLANE_H_*/
