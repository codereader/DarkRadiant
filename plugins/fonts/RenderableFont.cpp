#include "RenderableFont.h"

namespace fonts
{

RenderableFont::RenderableFont(const FontInfoPtr& font, Resolution res) :
	_font(font),
	_resolution(res)
{
	// Ensure the font we're using is realised, i.e. all shaders are constructed
	realiseFontShaders();
}

void RenderableFont::setResolution(Resolution res)
{
	if (_resolution != res)
	{
		_resolution = res;
		realiseFontShaders();
	}
}

void RenderableFont::realiseFontShaders()
{
	_font->glyphSets[_resolution]->realiseShaders();
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
