#include "RenderableText.h"

#include "itextstream.h"

#include "registry/registry.h"
#include "math/Matrix4.h"
#include <vector>
#include <list>
#include "gamelib.h"
#include "string/split.h"

#include "TextParts.h"
#include "GuiWindowDef.h"
#include "math/FloatTools.h"

namespace gui
{

	namespace
	{
		const std::string GKEY_SMALLFONT_LIMIT("/defaults/guiSmallFontLimit");
		const std::string GKEY_MEDIUMFONT_LIMIT("/defaults/guiMediumFontLimit");
	}

RenderableText::RenderableText(const IGuiWindowDef& owner) :
	_owner(owner)
{}

void RenderableText::printMissingGlyphSetError() const
{
    rWarning() << "[dm.gui] Font '" << _font->getName() << "'"
              << " does not have glyph set for resolution "
              << _resolution << std::endl;
}

void RenderableText::realiseFontShaders()
{
	while (_resolution < fonts::Resolution::NumResolutions)
	{
		fonts::IGlyphSetPtr glyphSet = _font->getGlyphSet(_resolution);

		if (glyphSet)
		{
			glyphSet->realiseShaders();
			break;
		}
		else
		{
			switch (_resolution)
			{
				case fonts::Resolution12: 
					rWarning() << "Falling back to higher resolution 24..." << std::endl;
					_resolution = fonts::Resolution24; 
					break;
				case fonts::Resolution24: 
					rWarning() << "Falling back to higher resolution 48..." << std::endl;
					_resolution = fonts::Resolution48; 
					break;
				case fonts::Resolution48: 
					rWarning() << "No resolutions to fall back." << std::endl;
					printMissingGlyphSetError();
					return;
				default:
					break;
			}
		}
	}
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

	std::string text = _owner.text;

	typedef std::vector<TextLinePtr> TextLines;
	TextLines lines;

	// Split the text into paragraphs
	std::vector<std::string> paragraphs;
	string::split(paragraphs, text, "\n", false);

    fonts::IGlyphSetPtr gsp = _font->getGlyphSet(_resolution);
    if (!gsp)
    {
        printMissingGlyphSetError();
        return;
    }

	fonts::IGlyphSet& glyphSet = *gsp;

	// Calculate the final scale of the glyphs
	float scale = _owner.textscale * glyphSet.getGlyphScale();

	// We need the maximum glyph height of the highest resolution font to calculate the line width
	std::size_t maxGlyphHeight = _font->getGlyphSet(fonts::Resolution48)->getMaxGlyphHeight();

	// Calculate the line height, this is usually max glyph height + 5 pixels
	double lineHeight = lrint(_owner.textscale * maxGlyphHeight + 5);

	// The distance from the top of the rectangle to the baseline
	double startingBaseLine = lrint(_owner.textscale * maxGlyphHeight + 2) + _owner.textaligny;

	Vector4 ownerRec = _owner.rect.getValue();

	for (std::size_t p = 0; p < paragraphs.size(); ++p)
	{
		// Check if more lines are possible, if at least one line is in the rectangle
		if (!lines.empty())
		{
			double curYPos = lineHeight * lines.size() + startingBaseLine;
			if (curYPos > ownerRec[3] - 2) break;
		}

		// Split the paragraphs into words
		std::list<std::string> words;
		string::split(words, paragraphs[p], " \t", false);

		// Add the words to lines
		TextLinePtr curLine(new TextLine(ownerRec[2] - 2 - _owner.textalignx, scale));

		while (!words.empty())
		{
			// If nowrap set to true, force words into this line
			bool added = curLine->addWord(words.front(), glyphSet, _owner.nowrap);

			if (added)
			{
				words.pop_front();

				if (!words.empty())
				{
					// Add a space after each word (noclipped) if more words are following
					curLine->addChar(' ', glyphSet, true);
				}

				continue;
			}

			// Not added
			if (curLine->empty())
			{
				// Line empty, but still not fitting, do it character-wise
				std::string word = words.front();
				words.pop_front();

				while (!word.empty())
				{
					// Take the first character
					if (curLine->addChar(word[0], glyphSet))
					{
						// Character added, remove from string
						word.erase(0, 1);
						continue;
					}
					else
					{
						// Not enough space, add one more character then break
						// stgatilov: that's for compability with Doom 3 engine
						// see also: #5914 and https://forums.thedarkmod.com/index.php?/topic/21710-implicit-linebreaks-in-text/
						curLine->addChar(word[0], glyphSet, true);
						word.erase(0, 1);
						break;
					}
				}

				// add the rest of the word to the front of the queue
				if (!word.empty())
				{
					words.push_front(word);
				}
			}

			// Trim any extra space from the end of the line
			curLine->removeTrailingSpace();

			// Line finished, consider alignment and vertical offset
			curLine->offset(Vector2(
				getAlignmentCorrection(curLine->getWidth()) + _owner.textalignx, // horizontal correction
				lineHeight * lines.size() + startingBaseLine // vertical correction
			));

			lines.push_back(curLine);

			// Clear the current line
			curLine = TextLinePtr();

			// Check if more lines are possible
			double curYPos = lineHeight * lines.size() + startingBaseLine;
			if (curYPos > ownerRec[3] - 2) break;

			// Allocate a new line, but only if we have any more words in this paragraph
			if (!words.empty())
			{
				curLine = TextLinePtr(new TextLine(ownerRec[2] - 2 - _owner.textalignx, scale));
			}
		}

		if (curLine != NULL)
		{
			// Trim any extra space from the end of the line
			curLine->removeTrailingSpace();

			// Add that line we started, even if it's an empty one
			curLine->offset(
				Vector2(
					getAlignmentCorrection(curLine->getWidth()) + _owner.textalignx,
					lineHeight * lines.size() +  + startingBaseLine
				)
			);

			lines.push_back(curLine);
		}
	}

	// Now sort the aligned characters into separate renderables, one per shader
	for (TextLines::const_iterator line = lines.begin(); line != lines.end(); ++line)
	{
		// Move the lines into our GUI rectangle
		(*line)->offset(Vector2(ownerRec[0], ownerRec[1]));

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
		// Somehow D3 adds a 2 pixel offset to the left (see idSimpleWindow::CalcClientRect)
		xoffset = 2;
		break;
	case 1: // center
		// Somehow D3 adds a 1 pixel offset to the left
		xoffset = 1 + (_owner.rect.getValue()[2] - lineWidth) / 2;
		break;
	case 2: // right
		xoffset = _owner.rect.getValue()[2] - 2 - lineWidth;
		break;
	};

	return xoffset;
}

void RenderableText::ensureFont()
{
	if (_owner.font.getValue().empty()) return; // no font specified

	if (_font != NULL) return; // already realised

	// Cut off the "fonts/" part
	std::string font = _owner.font;
	string::replace_first(font, "fonts/", "");

	_font = GlobalFontManager().findFontInfo(font);

	if (_font == NULL)
	{
		rWarning() << "Cannot find font " << _owner.font.getValue()
			<< " in windowDef " << _owner.name << std::endl;
		return;
	}

	// Determine resolution
	if (_owner.textscale <= game::current::getValue<float>(GKEY_SMALLFONT_LIMIT))
	{
		_resolution = fonts::Resolution12;
	}
	else if (_owner.textscale <= game::current::getValue<float>(GKEY_MEDIUMFONT_LIMIT))
	{
		_resolution = fonts::Resolution24;
	}
	else
	{
		_resolution = fonts::Resolution48;
	}

	// Ensure that the font shaders are realised
	realiseFontShaders();
}

} // namespace
