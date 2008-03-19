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
	static void constructStatic() {
		m_state = GlobalShaderCache().capture("$CLIPPER_OVERLAY");
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

	void render(RenderStateFlags state) const {
		if ((state & RENDER_FILL) != 0) {
			m_winding.draw(state);
		}
		else {
			m_winding.drawWireframe();
		}
	}

	void render(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const {
		renderer.SetState(m_state, Renderer::eWireframeOnly);
		renderer.SetState(m_state, Renderer::eFullMaterials);
		renderer.addRenderable(*this, localToWorld);
	}
}; // class BrushClipPlane

#endif /*BRUSHCLIPPLANE_H_*/
