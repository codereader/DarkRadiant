#ifndef RenderableText_h__
#define RenderableText_h__

#include "irenderable.h"
#include "ifonts.h"
#include <map>
#include "RenderableCharacterBatch.h"

namespace gui
{

class GuiWindowDef;

class RenderableText
{
	// The owning windowDef
	const GuiWindowDef& _owner;

	// The character soup, arranged into OpenGLRenderables, sorted by shader
	typedef std::map<ShaderPtr, RenderableCharacterBatchPtr> CharBatches;
	CharBatches _charBatches;

	// The font we're rendering
	fonts::IFontInfoPtr _font;

	// The resolution we're working with
	fonts::Resolution _resolution;

private:
    void printMissingGlyphSetError() const;

public:
	// Construct a renderable for the text in the given windowDef
	RenderableText(const GuiWindowDef& owner);

	void render();

	// Re-construct this structure, called when the text in the owning windowDef has been changed
	void recompile();

private:
	void realiseFontShaders();
	void ensureFont();

	// Calculates the horizontal alignment correction for the given line width
	// relative to the (default) left-aligned state
	double getAlignmentCorrection(double lineWidth);
};

} // namespace

#endif // RenderableText_h__
