#pragma once

#include "ientity.h"
#include "irender.h"
#include <fmt/format.h>

namespace entity
{

/**
 * greebo: this is a class encapsulating the "_color" spawnarg
 * of entity, observing it and maintaining the corresponding shader.
 */
class ColourKey :
	public KeyObserver
{
private:
	ShaderPtr _wireShader;
	Vector3 _colour;

	RenderSystemWeakPtr _renderSystem;

public:
	ColourKey() :
		_colour(1,1,1)
	{
		captureShader();
	}

	const Vector3& getColour() const
	{
		return _colour;
	}

	// Called when "_color" keyvalue changes
	void onKeyValueChanged(const std::string& value)
	{
		// Initialise the colour with white, in case the string parse fails
		_colour[0] = _colour[1] = _colour[2] = 1;

		// Use a stringstream to parse the string
		std::stringstream strm(value);

		strm << std::skipws;
		strm >> _colour.x();
		strm >> _colour.y();
		strm >> _colour.z();

		captureShader();
	}

	void setRenderSystem(const RenderSystemPtr& renderSystem)
	{
		_renderSystem = renderSystem;

		captureShader();
	}

    /// Return a non-owning (possibly null) pointer to the Shader
    Shader* getWireShader() const
    {
        return _wireShader.get();
    }

private:

	void captureShader()
	{
		RenderSystemPtr renderSystem = _renderSystem.lock();

		if (renderSystem)
		{
			std::string wireCol = fmt::format("<{0:f} {1:f} {2:f}>", _colour[0], _colour[1], _colour[2]);
			_wireShader = renderSystem->capture(wireCol);
		}
		else
		{
			_wireShader.reset();
		}
	}
};

} // namespace entity
