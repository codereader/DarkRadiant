#pragma once

#include "math/Plane3.h"
#include "irender.h"
#include "irenderable.h"
#include "Winding.h"
#include "RenderableWinding.h"

class BrushClipPlane : 
    public render::RenderableWinding
{
private:
	Plane3 _plane;
	Winding _winding;
	ShaderPtr _shader;

public:
    BrushClipPlane() :
        RenderableWinding(_winding)
    {}

    virtual ~BrushClipPlane() {}

	void setPlane(const Brush& brush, const Plane3& plane, IRenderEntity& entity)
	{
		_plane = plane;

		if (_plane.isValid())
		{
			brush.windingForClipPlane(_winding, _plane);

            _winding.updateNormals(_plane.normal());

            // Update the RenderableWinding
            queueUpdate();
            update(_shader, entity);
		}
		else 
		{
			_winding.resize(0);
            clear();
		}
	}

#if 0
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
#endif

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

#if 0
	void render(IRenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const
	{
		collector.addRenderable(*_shader, *this, localToWorld);
	}
#endif
};
