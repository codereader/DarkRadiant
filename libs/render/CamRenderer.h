#pragma once

#include "render/RenderableCollectorBase.h"
#include "imap.h"
#include "ivolumetest.h"

namespace render
{

/// RenderableCollector for use with 3D camera views or preview widgets
class CamRenderer : 
    public RenderableCollectorBase
{
public:
    struct HighlightShaders
    {
        ShaderPtr faceHighlightShader;
        ShaderPtr primitiveHighlightShader;
        ShaderPtr mergeActionShaderAdd;
        ShaderPtr mergeActionShaderChange;
        ShaderPtr mergeActionShaderRemove;
        ShaderPtr mergeActionShaderConflict;
    };

private:
    // The VolumeTest object for object culling
    const VolumeTest& _view;

    IMap::EditMode _editMode;

    const HighlightShaders& _shaders;

public:

    /// Initialise CamRenderer with optional highlight shaders
    CamRenderer(const VolumeTest& view, const HighlightShaders& shaders)
    : _view(view),
      _editMode(GlobalMapModule().getEditMode()),
      _shaders(shaders)
    {}

    void prepare()
    {
        _editMode = GlobalMapModule().getEditMode();
    }

    void cleanup()
    {
    }

    // RenderableCollector implementation

    bool supportsFullMaterials() const override { return true; }

    void addHighlightRenderable(const OpenGLRenderable& renderable, const Matrix4& localToWorld) override
    {
        if (_editMode == IMap::EditMode::Merge && (_flags & Highlight::Flags::MergeAction) != 0)
        {
            const auto& mergeShader = (_flags & Highlight::Flags::MergeActionAdd) != 0 ? _shaders.mergeActionShaderAdd :
                (_flags & Highlight::Flags::MergeActionRemove) != 0 ? _shaders.mergeActionShaderRemove :
                (_flags & Highlight::Flags::MergeActionConflict) != 0 ? _shaders.mergeActionShaderConflict : _shaders.mergeActionShaderChange;

            if (mergeShader)
            {
                mergeShader->addRenderable(renderable, localToWorld);
            }
        }

        if ((_flags & Highlight::Flags::Primitives) != 0 && _shaders.primitiveHighlightShader)
        {
            _shaders.primitiveHighlightShader->addRenderable(renderable, localToWorld);
        }

        if ((_flags & Highlight::Flags::Faces) != 0 && _shaders.faceHighlightShader)
        {
            _shaders.faceHighlightShader->addRenderable(renderable, localToWorld);
        }
    }
};


}
