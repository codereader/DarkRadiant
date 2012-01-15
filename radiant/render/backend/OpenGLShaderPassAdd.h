#pragma once

#include "OpenGLShaderPass.h"

namespace render
{

/**
 * Functor class which populates an OpenGLShaderPass.
 */
class OpenGLShaderPassAdd
{
private:
	OpenGLShaderPass& _pass;
	const OpenGLRenderable& _renderable;
	const Matrix4& _modelview;
	const IRenderEntity* _entity;

public:
	OpenGLShaderPassAdd(OpenGLShaderPass& pass, 
						const OpenGLRenderable& renderable, 
						const Matrix4& modelview, 
						const IRenderEntity* entity = NULL) :
		_pass(pass), 
		_renderable(renderable), 
		_modelview(modelview),
		_entity(entity)
	{}

	void visit(const RendererLight& light)
	{
		if (_entity)
		{
			_pass.addRenderable(_renderable, _modelview, *_entity, &light);
		}
		else
		{
			_pass.addRenderable(_renderable, _modelview, &light);
		}
	}
};

}
