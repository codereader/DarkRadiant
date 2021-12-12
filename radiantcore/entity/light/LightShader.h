#pragma once

#include <string>
#include "irender.h"

namespace entity {

/// Wrapper for a Shader used to render a Light
class LightShader: public sigc::trackable
{
private:
	std::string _shaderName;

	ShaderPtr _shader;

	// The reference to the rendersystem, in case our shader is changing
	RenderSystemWeakPtr _renderSystem;

public:
	static std::string m_defaultShader;

	LightShader() :
		_shaderName(m_defaultShader)
	{}

	void valueChanged(const std::string& value)
	{
		_shaderName = value.empty() ? m_defaultShader : value;

		captureShader();
		SceneChangeNotify();
	}

	void setRenderSystem(const RenderSystemPtr& renderSystem)
	{
		_renderSystem = renderSystem;

		captureShader();
	}

	const ShaderPtr& get() const
	{
		return _shader;
	}

private:

	void captureShader()
	{
		RenderSystemPtr renderSystem = _renderSystem.lock();

		if (renderSystem)
		{
			_shader = renderSystem->capture(_shaderName);
		}
		else
		{
			_shader.reset();
		}
	}
};

} // namespace entity
