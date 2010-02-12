#include "RenderableFont.h"

namespace fonts
{

RenderableFont::RenderableFont(const FontInfoPtr& font) :
	_font(font)
{
	// Ensure the font we're using is realised, i.e. all shaders are constructed
	
}

void RenderableFont::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
	// TODO
}

void RenderableFont::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
	renderSolid(collector, volume);
}

void RenderableFont::renderText(const std::string& text, const Vector3& pos)
{
	// Add ourselves as renderable
}

void RenderableFont::renderText(const std::string& text, 
	const Vector3& areaTopLeft, const Vector3& areaBottomRight)
{
	// Add ourselves as renderable
}

} // namespace fonts
