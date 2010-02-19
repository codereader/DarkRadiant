#include "RenderableText.h"

#include "itextstream.h"

#include "math/matrix.h"
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

void RenderableText::render()
{
	// Add each renderable character batch to the collector
	for (CharBatches::const_iterator i = _charBatches.begin(); i != _charBatches.end(); ++i)
	{
		// Switch to this shader
		glBindTexture(GL_TEXTURE_2D, i->first->getMaterial()->getEditorImage()->getGLTexNum());

		// Submit geometry
		i->second->render();
	}
}

void RenderableText::recompile()
{
	_charBatches.clear();
	
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

			// Line finished, consider alignment and vertical offset
			curLine->offset(Vector2(
				getAlignmentCorrection(curLine->getWidth()), // horizontal correction
				lineHeight * lines.size()					 // vertical correction
			));

			lines.push_back(curLine);

			// Allocate a new line, and proceed
			curLine = TextLinePtr(new TextLine(_owner.rect[2]));
		}

		// Add that line we started
		if (!curLine->empty())
		{
			curLine->offset(
				Vector2(getAlignmentCorrection(curLine->getWidth()), lineHeight * lines.size())
			);
			lines.push_back(curLine);
		}
	}

	// Now sort the aligned characters into separate renderables, one per shader
	for (TextLines::const_iterator line = lines.begin(); line != lines.end(); ++line)
	{
		// Move the lines into our GUI rectangle
		(*line)->offset(Vector2(_owner.rect[0], _owner.rect[1]));

		for (TextLine::Chars::const_iterator c = (*line)->getChars().begin();
			 c != (*line)->getChars().end(); ++c)
		{
			CharBatches::iterator batch = _charBatches.find(c->glyph->shader);

			if (batch == _charBatches.end())
			{
				RenderableCharacterBatchPtr b(new RenderableCharacterBatch);

				std::pair<CharBatches::iterator, bool> result = _charBatches.insert(
					CharBatches::value_type(c->glyph->shader, b)
				);

				batch = result.first;
			}

			batch->second->addGlyph(*c);
		}
	}

	// Compile the vertex buffer objects
	for (CharBatches::iterator i = _charBatches.begin(); i != _charBatches.end(); ++i)
	{
		i->second->compile();
	}
}

double RenderableText::getAlignmentCorrection(double lineWidth)
{
	double xoffset = 0;

	switch (_owner.textalign)
	{
	case 0: // left
		xoffset = 0;
		break;
	case 1: // center
		xoffset = (_owner.rect[2] - lineWidth) / 2;
		break;
	case 2: // right
		xoffset = _owner.rect[2] - lineWidth;
		break;
	};

	return xoffset;
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
