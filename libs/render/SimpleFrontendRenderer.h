#pragma once

#include "irenderable.h"
#include "irender.h"
#include <list>

namespace render
{

/**
 * greebo: This is a basic front-end renderer (collecting renderables)
 * No highlighting support. It is returning FullMaterials as renderer style.
 */
class SimpleFrontendRenderer :
	public RenderableCollector
{
public:
	SimpleFrontendRenderer()
	{}

	void addRenderable(const ShaderPtr& shader, const OpenGLRenderable& renderable, const Matrix4& world) override
	{
		shader->addRenderable(renderable, world);
	}

	void addRenderable(const ShaderPtr& shader, const OpenGLRenderable& renderable,
		const Matrix4& world, const IRenderEntity& entity) override
	{
		shader->addRenderable(renderable, world, entity);
	}

	void addRenderable(const ShaderPtr& shader, const OpenGLRenderable& renderable,
		const Matrix4& world, const IRenderEntity& entity, const LightList& lights) override
	{
		shader->addRenderable(renderable, world, entity, &lights);
	}

	bool supportsFullMaterials() const override
	{
        return true;
	}

    // No support for selection highlighting
	void setHighlightFlag(Highlight::Flags flags, bool enabled) override {}
};

} // namespace
