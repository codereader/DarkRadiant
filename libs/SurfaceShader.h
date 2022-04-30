#pragma once

#include <string>
#include <sigc++/connection.h>
#include "debugging/debugging.h"
#include "util/Noncopyable.h"
#include "irender.h"
#include "shaderlib.h"

/**
 * Encapsulates a GL ShaderPtr and keeps track whether this
 * shader is actually in use in the map or not. The shader
 * is captured and released based on whether there is a
 * shadersystem reference available.
 */
class SurfaceShader :
    public util::Noncopyable,
	public Shader::Observer
{
private:
    // greebo: The name of the material
    std::string _materialName;

    RenderSystemPtr _renderSystem;

    ShaderPtr _glShader;

    // In-use flag
    bool _inUse;

    bool _realised;

	// Client signals
	std::function<void()> _callbackRealised;
	std::function<void()> _callbackUnrealised;

public:
    // Constructor. The renderSystem reference will be kept internally as reference
    // The SurfaceShader will try to de-reference it when capturing shaders.
    SurfaceShader(const std::string& materialName, const RenderSystemPtr& renderSystem = RenderSystemPtr()) :
        _materialName(materialName),
        _renderSystem(renderSystem),
        _inUse(false),
        _realised(false)
    {
        captureShader();
    }

    // Destructor
    virtual ~SurfaceShader()
    {
        releaseShader();
    }

    /**
    * Indicates whether this Shader is actually in use in the scene or not.
    * The shader is not in use if the owning Patch resides on the UndoStack, forex.
    */
    void setInUse(bool isUsed)
    {
        _inUse = isUsed;

        if (!_glShader) return;

        // Update the shader's use count
        if (_inUse)
        {
            _glShader->incrementUsed();
        }
        else
        {
            _glShader->decrementUsed();
        }
    }

    /**
    * \brief
    * Get the material name.
    */
    const std::string& getMaterialName() const
    {
        return _materialName;
    }

    /**
    * \brief
    * Set the material name.
    */
    void setMaterialName(const std::string& name)
    {
        // return, if the shader is the same as the currently used
        if (shader_equal(_materialName, name)) return;

        releaseShader();

        _materialName = name;

        captureShader();
    }

    /**
    * \brief
    * Return the Shader for rendering.
    */
    const ShaderPtr& getGLShader() const
    {
        return _glShader;
    }

    // Return the dimensions of the editorimage of the contained material
    std::size_t getWidth() const
    {
        if (_realised)
        {
            return _glShader->getMaterial()->getEditorImage()->getWidth();
        }

        return 1;
    }

    std::size_t getHeight() const
    {
        if (_realised)
        {
            return _glShader->getMaterial()->getEditorImage()->getHeight();
        }

        return 1;
    }

    float getTextureAspectRatio() const
    {
        return static_cast<float>(getWidth()) / getHeight();
    }

    void setRenderSystem(const RenderSystemPtr& renderSystem)
    {
        _renderSystem = renderSystem;

        captureShader();
    }

	// Set the functor which is invoked when this shader is realised
	void setRealisedCallback(const std::function<void()>& callback)
	{
		_callbackRealised = callback;
	}

	// Set the functor which is invoked when this shader is unrealised
	void setUnrealisedCallback(const std::function<void()>& callback)
	{
		_callbackUnrealised = callback;
	}

	// Called when the GL shader is realised
    void realise()
    {
        assert(!_realised);
        _realised = true;

        if (_callbackRealised)
        {
            _callbackRealised();
        }
    }

	// Called when the GL shader is unrealised
    void unrealise()
    {
        assert(_realised);

        if (_callbackUnrealised)
        {
            _callbackUnrealised();
        }

        _realised = false;
    }

	bool isRealised() const
	{
		return _realised;
	}

private:
    // Shader capture and release
    void captureShader()
    {
		// Release previous resources in any case
		releaseShader();

        // Check if we have a rendersystem - can we capture already?
        if (_renderSystem)
        {
            _glShader = _renderSystem->capture(_materialName);
            assert(_glShader);

			_glShader->attachObserver(*this);

            if (_inUse)
            {
                _glShader->incrementUsed();
            }
        }
    }

    void releaseShader()
    {
        if (_glShader)
        {
			_glShader->detachObserver(*this);

            if (_inUse)
            {
                _glShader->decrementUsed();
            }

            _glShader.reset();
        }
    }

	// Inherited via Observer
	void onShaderRealised() override
	{
		realise();
	}

	void onShaderUnrealised() override
	{
		unrealise();
	}
};
