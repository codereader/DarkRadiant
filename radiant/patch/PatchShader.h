#pragma once

#include "irender.h"
#include "moduleobservers.h"
#include <boost/noncopyable.hpp>

class Patch;

class PatchShader :
    public ModuleObserver,
    public boost::noncopyable
{
private:
    Patch& _owner;

    // greebo: The name of the material
    std::string _materialName;

    ShaderPtr _glShader;

    // In-use flag
    bool _inUse;

    bool _realised;

public:
    class SavedState
    {
    public:
        std::string _materialName;

        SavedState(const PatchShader& owner) :
            _materialName(owner.getMaterialName())
        {}

        void exportState(PatchShader& owner) const
        {
            owner.setMaterialName(_materialName);
        }
    };

    // Constructor
    PatchShader(Patch& owner, const std::string& materialName);

    // Destructor
    virtual ~PatchShader();

    /**
    * Indicates whether this Shader is actually in use in the scene or not.
    * The shader is not in use if the owning Patch resides on the UndoStack, forex.
    */
    void setInUse(bool isUsed);

    /**
    * \brief
    * Get the material name.
    */
    const std::string& getMaterialName() const;

    /**
    * \brief
    * Set the material name.
    */
    void setMaterialName(const std::string& name);

    /**
    * \brief
    * Return the Shader for rendering.
    */
    const ShaderPtr& getGLShader() const;

    // Return the dimensions of the editorimage of the contained material
    std::size_t getWidth() const;
    std::size_t getHeight() const;

    void setRenderSystem(const RenderSystemPtr& renderSystem);

    // ModuleObserver methods
    void realise() override;
    void unrealise() override;

private:
    // Shader capture and release
    void captureShader();
    void releaseShader();
};
