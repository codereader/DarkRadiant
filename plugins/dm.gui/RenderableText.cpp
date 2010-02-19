#include "RenderableText.h"

#include "itextstream.h"

#include <vector>
#include <list>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "TextParts.h"
#include "GuiWindowDef.h"

namespace gui
{

RenderableText::RenderableText(const GuiWindowDef& owner) :
	_owner(owner)
{}

void RenderableText::realiseFontShaders()
{
	_font->getGlyphSet(_resolution)->realiseShaders();
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
	ensureFont();

	if (_font == NULL) return; // Rendering not possible

	std::string text = _owner.getText();

	typedef std::vector<TextLinePtr> TextLines;
	TextLines lines;

	// Split the text into paragraphs
	std::vector<std::string> paragraphs;
	boost::algorithm::split(paragraphs, text, boost::algorithm::is_any_of("\n"));

	double lineHeight = 16;

	for (std::size_t p = 0; p < paragraphs.size(); ++p)
	{
		// Split the paragraphs into words
		std::list<std::string> words;
		boost::algorithm::split(words, text, boost::algorithm::is_any_of(" \t"));

		// Add the words to lines
		TextLinePtr curLine(new TextLine(_owner.rect[2]));

		while (!words.empty())
		{
			bool added = curLine->addWord(words.front(), *_font->getGlyphSet(_resolution));

			if (added)
			{
				words.pop_front();
				continue;
			}

			// Not added
			if (curLine->empty())
			{
				// Line empty, but still not fitting, force it
				curLine->addWord(words.front(), *_font->getGlyphSet(_resolution), true);
				words.pop_front();
			}

			// Line finished, proceed to next
			// TODO: consider textalign
			curLine->offset(Vector2(0, lineHeight * lines.size()));
			lines.push_back(curLine);

			// Allocate a new line, and proceed
			curLine = TextLinePtr(new TextLine(_owner.rect[2]));
		}

		// Add that line we started
		if (!curLine->empty())
		{
			curLine->offset(Vector2(0, lineHeight * lines.size()));
			lines.push_back(curLine);
		}
	}
}

void RenderableText::ensureFont()
{
	if (_font != NULL) return; // already realised

	_font = GlobalFontManager().findFontInfo(_owner.font);

	if (_font == NULL)
	{
		globalWarningStream() << "Cannot find font " << _owner.font 
			<< " in windowDef " << _owner.name << std::endl;
		return;
	}

	// Determine resolution (TODO: Use owner textscale to determine resolution)
	_resolution = fonts::Resolution24;

	// Ensure that the font shaders are realised
	realiseFontShaders();
}

} // namespace
