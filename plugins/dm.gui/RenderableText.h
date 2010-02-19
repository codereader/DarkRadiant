#ifndef RenderableText_h__
#define RenderableText_h__

#include "irenderable.h"
#include "ifonts.h"

namespace gui
{

class GuiWindowDef;

class RenderableText :
	public Renderable
{
private:
	// The owning windowDef
	const GuiWindowDef& _owner;

	// The current state (used during front end rendering)
	mutable MaterialPtr _curState;

	// The font we're rendering
	fonts::IFontInfoPtr _font;

	// The resolution we're working with
	fonts::Resolution _resolution;

public:
	// Construct a renderable for the text in the given windowDef
	RenderableText(const GuiWindowDef& owner);

	// Renderable implementation, adds OpenGLRenderables to the collector
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const;

	// Re-construct this structure, called when the text in the owning windowDef has been changed
	void recompile();

private:
	void realiseFontShaders();
	void ensureFont();
};

} // namespace

#endif // RenderableText_h__
