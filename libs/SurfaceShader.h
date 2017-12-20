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
    public util::Noncopyable
{
private:
    // greebo: The name of the material
    std::string _materialName;

    RenderSystemPtr _renderSystem;

    ShaderPtr _glShader;

    // In-use flag
    bool _inUse;

    bool _realised;

	// Signals connected to the contained GL shader
	sigc::connection _glShaderRealised;
	sigc::connection _glShaderUnrealised;

	// Client signals
	sigc::signal<void> _signalRealised;
	sigc::signal<void> _signalUnrealised;

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

    void setRenderSystem(const RenderSystemPtr& renderSystem)
    {
        _renderSystem = renderSystem;

        captureShader();
    }

	// Get the signal which is fired when this shader is realised
	sigc::signal<void>& signal_Realised()
	{
		return _signalRealised;
	}

	// Get the signal which is fired when this shader is unrealised
	sigc::signal<void>& signal_Unrealised()
	{
		return _signalUnrealised;
	}

	// Called when the GL shader is realised
    void realise()
    {
        assert(!_realised);
        _realised = true;

		signal_Realised().emit();
    }

	// Called when the GL shader is unrealised
    void unrealise()
    {
        assert(_realised);

		signal_Unrealised().emit();

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

			_glShaderRealised = _glShader->signal_Realised().connect(
				sigc::mem_fun(*this, &SurfaceShader::realise)
			);
			_glShaderUnrealised = _glShader->signal_Unrealised().connect(
				sigc::mem_fun(*this, &SurfaceShader::unrealise)
			);

			// Realise right now if the GLShader is already in that state
			if (_glShader->isRealised())
			{
				realise();
			}

            if (_inUse)
            {
                _glShader->incrementUsed();
            }
        }
    }

    void releaseShader()
    {
		_glShaderRealised.disconnect();
		_glShaderUnrealised.disconnect();

        if (_glShader)
        {
			if (_glShader->isRealised())
			{
				unrealise();
			}

            if (_inUse)
            {
                _glShader->decrementUsed();
            }

            _glShader.reset();
        }
    }
};
