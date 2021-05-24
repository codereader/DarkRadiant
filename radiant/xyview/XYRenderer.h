#pragma once

#include "irenderable.h"

/// RenderableCollector implementation for the ortho view
class XYRenderer: public RenderableCollector
{
    // State type structure
    struct State
    {
        bool highlightPrimitives;
        bool highlightAsGroupMember;
        bool highlightAsMergeAction;

        // Constructor
        State() : 
            highlightPrimitives(false), 
            highlightAsGroupMember(false),
            highlightAsMergeAction(false)
        {}
    };

    State _state;
    RenderStateFlags _globalstate;

    // Shader to use for highlighted objects
    Shader* _selectedShader;
    Shader* _selectedShaderGroup;
    Shader* _mergeActionShader;

public:
    XYRenderer(RenderStateFlags globalstate, Shader* selected, Shader* selectedGroup, Shader* mergeActionShader) :
        _globalstate(globalstate),
        _selectedShader(selected),
        _selectedShaderGroup(selectedGroup),
        _mergeActionShader(mergeActionShader)
    {}

    bool supportsFullMaterials() const override
    {
        return false;
    }

    void setHighlightFlag(Highlight::Flags flags, bool enabled) override
    {
        if (flags & Highlight::Primitives)
        {
            _state.highlightPrimitives = enabled;
        }

        if (flags & Highlight::GroupMember)
        {
            _state.highlightAsGroupMember = enabled;
        }

        if (flags & Highlight::MergeAction)
        {
            _state.highlightAsMergeAction = enabled;
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
        if (_state.highlightAsMergeAction)
        {
            _mergeActionShader->addRenderable(renderable, localToWorld, nullptr, entity);
        }
        else if (_state.highlightPrimitives)
        {
            if (_state.highlightAsGroupMember)
                _selectedShaderGroup->addRenderable(renderable, localToWorld,
                                                    nullptr, entity);
            else
                _selectedShader->addRenderable(renderable, localToWorld, nullptr, entity);
        }

        shader.addRenderable(renderable, localToWorld, nullptr, entity);
    }

    void render(const Matrix4& modelview, const Matrix4& projection)
    {
        GlobalRenderSystem().render(_globalstate, modelview, projection, Vector3(0,0,0));
    }
}; // class XYRenderer
