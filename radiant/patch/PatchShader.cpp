#include "PatchShader.h"

#include "irender.h"
#include "Patch.h"
#include "shaderlib.h"

PatchShader::PatchShader(Patch& owner, const std::string& materialName) :
    _owner(owner),
    _materialName(materialName),
    _inUse(false),
    _realised(false)
{
    captureShader();
}

PatchShader::~PatchShader()
{
    releaseShader();
}

void PatchShader::setInUse(bool inUse)
{
    _inUse = inUse;

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

void PatchShader::captureShader()
{
    // Check if we have a rendersystem - can we capture already?
    RenderSystemPtr renderSystem = _owner.getRenderSystem();

    if (renderSystem)
    {
        releaseShader();

        _glShader = renderSystem->capture(_materialName);
        assert(_glShader);

        _glShader->attach(*this);

        if (_inUse)
        {
            _glShader->incrementUsed();
        }
    }
    else
    {
        releaseShader();
    }
}

void PatchShader::releaseShader()
{
    if (_glShader)
    {
        if (_inUse)
        {
            _glShader->decrementUsed();
        }

        _glShader->detach(*this);

        _glShader.reset();
    }
}

void PatchShader::realise()
{
    assert(!_realised);
    _realised = true;

    //std::for_each(_observers.begin(), _observers.end(), FaceShaderObserverRealise());
}

void PatchShader::unrealise()
{
    assert(_realised);

    //std::for_each(_observers.begin(), _observers.end(), FaceShaderObserverUnrealise());

    _realised = false;
}

const std::string& PatchShader::getMaterialName() const
{
    return _materialName;
}

void PatchShader::setMaterialName(const std::string& name)
{
    // return, if the shader is the same as the currently used
    if (shader_equal(_materialName, name)) return;

    releaseShader();

    _materialName = name;

    captureShader();
}

const ShaderPtr& PatchShader::getGLShader() const
{
    return _glShader;
}

std::size_t PatchShader::getWidth() const
{
    if (_realised)
    {
        return _glShader->getMaterial()->getEditorImage()->getWidth();
    }

    return 1;
}

std::size_t PatchShader::getHeight() const 
{
    if (_realised)
    {
        return _glShader->getMaterial()->getEditorImage()->getHeight();
    }

    return 1;
}

void PatchShader::setRenderSystem(const RenderSystemPtr& renderSystem)
{
    captureShader();
}
