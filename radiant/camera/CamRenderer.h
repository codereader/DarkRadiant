#pragma once

#include "irenderable.h"
#include "irender.h"

/// Implementation of RenderableCollector for the 3D camera view
class CamRenderer : 
	public RenderableCollector
{
    // Data class storing state on the stack
	struct State
	{
        bool highlightFaces;
        bool highlightPrimitives;
		Shader* shader; // raw pointer for speed
		const LightList* lights;

		State() : highlightFaces(false),
                  highlightPrimitives(false),
                  shader(NULL),
                  lights(NULL) 
        {}
	};

    // Stack of states
    std::vector<State> _stateStack;

    RenderStateFlags m_globalstate;
    ShaderPtr _highlightedPrimitiveShader;
    ShaderPtr _highlightedFaceShader;
    const Vector3& m_viewer;

public:

    /**
     * \brief
     * Initialise a CamRenderer
     *
     * \param globalstate
     * Global RenderStateFlags active for this render pass.
     */
    CamRenderer(RenderStateFlags globalstate,
                const ShaderPtr& primHighlightShader,
                const ShaderPtr& faceHighlightShader,
                const Vector3& viewer);

    void render(const Matrix4& modelview, const Matrix4& projection);

    // RenderableCollector implementation
    void SetState(const ShaderPtr& shader, EStyle style) override;
    bool supportsFullMaterials() const override;
    void PushState() override;
    void PopState() override;
	void setHighlightFlag(Highlight::Flags flags, bool enabled) override;
    void setLights(const LightList& lights) override;
    void addRenderable(const OpenGLRenderable& renderable, const Matrix4& world) override;
	void addRenderable(const OpenGLRenderable& renderable,
                       const Matrix4& world,
                       const IRenderEntity& entity) override;
};
