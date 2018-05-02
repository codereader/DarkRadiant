#pragma once

#include "math/Plane3.h"
#include "irender.h"
#include "irenderable.h"
#include "Winding.h"

class BrushClipPlane : 
	public OpenGLRenderable
{
private:
	Plane3 _plane;
	Winding _winding;
	ShaderPtr _shader;

public:
    virtual ~BrushClipPlane() {}

	void setPlane(const Brush& brush, const Plane3& plane)
	{
		_plane = plane;

		if (_plane.isValid())
		{
			brush.windingForClipPlane(_winding, _plane);
		}
		else 
		{
			_winding.resize(0);
		}

		_winding.updateNormals(_plane.normal());
	}

	void render(const RenderInfo& info) const override
	{
		if (info.checkFlag(RENDER_FILL))
		{
			_winding.render(info);
		}
		else
		{
			_winding.drawWireframe();
		}
	}

	void setRenderSystem(const RenderSystemPtr& renderSystem)
	{
		if (renderSystem)
		{
			_shader = renderSystem->capture("$CLIPPER_OVERLAY");
		}
		else
		{
			_shader.reset();
		}
	}

	void render(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const
	{
		collector.addRenderable(_shader, *this, localToWorld);
	}
};
