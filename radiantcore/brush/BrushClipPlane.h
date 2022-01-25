#pragma once

#include "math/Plane3.h"
#include "irender.h"
#include "irenderable.h"
#include "Winding.h"
#include "RenderableWinding.h"

class BrushClipPlane final :
    public render::RenderableWinding
{
private:
	Winding _winding;
	ShaderPtr _shader;

public:
    BrushClipPlane() :
        RenderableWinding(_winding)
    {}

	void setPlane(const Brush& brush, const Plane3& plane, IRenderEntity& entity)
	{
		if (plane.isValid())
		{
			brush.windingForClipPlane(_winding, plane);

            _winding.updateNormals(plane.normal());

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

	void setRenderSystem(const RenderSystemPtr& renderSystem)
	{
		if (renderSystem)
		{
			_shader = renderSystem->capture(BuiltInShaderType::BrushClipPlane);
		}
		else
		{
			_shader.reset();
		}
	}
};
