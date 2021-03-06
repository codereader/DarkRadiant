#pragma once

#include "irender.h"
#include "irenderable.h"
#include "imap.h"

/// RenderableCollector implementation for the ortho view
class XYRenderer : 
    public RenderableCollector
{
public:
    struct HighlightShaders
    {
        ShaderPtr selectedShader;
        ShaderPtr selectedShaderGroup;
        ShaderPtr mergeActionShaderAdd;
        ShaderPtr mergeActionShaderChange;
        ShaderPtr mergeActionShaderRemove;
        ShaderPtr mergeActionShaderConflict;
        ShaderPtr nonMergeActionNodeShader;
    };

private:
    std::size_t _flags;
    RenderStateFlags _globalstate;

    IMap::EditMode _editMode;

    const HighlightShaders& _shaders;

public:
    XYRenderer(RenderStateFlags globalstate, const HighlightShaders& shaders) :
        _flags(Highlight::Flags::NoHighlight),
        _globalstate(globalstate),
        _editMode(GlobalMapModule().getEditMode()),
        _shaders(shaders)
    {}

    bool supportsFullMaterials() const override
    {
        return false;
    }

    void setHighlightFlag(Highlight::Flags flags, bool enabled) override
    {
        if (enabled)
        {
            _flags |= flags;
        }
        else
        {
            _flags &= ~flags;
        }
    }

    // Ortho view never processes lights
    void addLight(const RendererLight&) override {}

    void addRenderable(Shader& shader,
                       const OpenGLRenderable& renderable,
                       const Matrix4& localToWorld,
                       const LitObject* /* litObject */,
                       const IRenderEntity* entity = nullptr) override
    {
        if (_editMode == IMap::EditMode::Merge)
        {
            if (_flags & Highlight::Flags::MergeAction)
            {
                // This is a merge-relevant node that should be rendered in a special colour
                const auto& mergeShader = (_flags & Highlight::Flags::MergeActionAdd) != 0 ? _shaders.mergeActionShaderAdd :
                    (_flags & Highlight::Flags::MergeActionRemove) != 0 ? _shaders.mergeActionShaderRemove : 
                    (_flags & Highlight::Flags::MergeActionConflict) != 0 ? _shaders.mergeActionShaderConflict : _shaders.mergeActionShaderChange;

                if (mergeShader)
                {
                    mergeShader->addRenderable(renderable, localToWorld, nullptr, entity);
                }
            }
            else
            {
                // Everything else is using the shader for non-merge-affected nodes
                _shaders.nonMergeActionNodeShader->addRenderable(renderable, localToWorld, nullptr, entity);
            }

            // Elements can still be selected in merge mode
            if ((_flags & Highlight::Flags::Primitives) != 0)
            {
                _shaders.selectedShader->addRenderable(renderable, localToWorld, nullptr, entity);
            }

            return;
        }
        
        // Regular editing mode, add all highlighted nodes to the corresponding shader
        if ((_flags & Highlight::Flags::Primitives) != 0)
        {
            if ((_flags & Highlight::Flags::GroupMember) != 0)
            {
                _shaders.selectedShaderGroup->addRenderable(renderable, localToWorld, nullptr, entity);
            }
            else
            {
                _shaders.selectedShader->addRenderable(renderable, localToWorld, nullptr, entity);
            }
        }

        shader.addRenderable(renderable, localToWorld, nullptr, entity);
    }

    void render(const Matrix4& modelview, const Matrix4& projection)
    {
        GlobalRenderSystem().render(_globalstate, modelview, projection, Vector3(0,0,0));
    }
}; // class XYRenderer
