#include "RenderableText.h"

namespace gui
{

RenderableText::RenderableText(const GuiWindowDef& owner) :
	_owner(owner)
{}

void RenderableText::realiseFontShaders()
{
	/*_curState.reset();
	_font->glyphSets[_resolution]->realiseShaders();*/
}

void RenderableText::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
	// TODO
}

void RenderableText::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
	renderSolid(collector, volume);
}

void RenderableText::recompile()
{
	// TODO
}

} // namespace
