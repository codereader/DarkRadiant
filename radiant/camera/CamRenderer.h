#pragma once

#include "irenderable.h"
#include "irender.h"

/// Implementation of RenderableCollector for the 3D camera view
class CamRenderer : 
	public RenderableCollector
{
private:
    // Data class storing state on the stack
	struct State
	{
        bool highlightFaces;
        bool highlightPrimitives;

		State() : 
			highlightFaces(false),
			highlightPrimitives(false)
        {}
	};

    // Highlight state
    State _state;

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
    bool supportsFullMaterials() const override;
	void setHighlightFlag(Highlight::Flags flags, bool enabled) override;

	void addRenderable(const ShaderPtr& shader, const OpenGLRenderable& renderable, const Matrix4& world) override;
	void addRenderable(const ShaderPtr& shader, const OpenGLRenderable& renderable,
		const Matrix4& world, const IRenderEntity& entity) override;
	void addRenderable(const ShaderPtr& shader, const OpenGLRenderable& renderable,
		const Matrix4& world, const IRenderEntity& entity, const LightList& lights) override;
};
