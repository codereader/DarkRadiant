#ifndef TextParts_h__
#define TextParts_h__

#include "ifonts.h"
#include "math/Vector2.h"
#include <string>

namespace gui
{

// A single 2D vertex, including U/V coordinates
struct Vertex2D
{
	Vector2 vertex;
	Vector2 texcoord;
};

class TextChar
{
private:
	// The actual character
	char _char;

	// The font scale needed for the width calculation
	float _fontScale;
public:
	fonts::IGlyphInfoPtr glyph;

	// The 4 coordinates, marking the quad (3D and UV)
	Vertex2D coords[4];

	TextChar(unsigned char c, fonts::IGlyphSet& glyphs, float fontScale) :
		_char(c),
		_fontScale(fontScale)
	{
		glyph = glyphs.getGlyph(static_cast<std::size_t>(c));

		coords[0].vertex = Vector2(0, -glyph->top * _fontScale);
		coords[1].vertex = Vector2(glyph->imageWidth * _fontScale, -glyph->top * _fontScale);
		coords[2].vertex = Vector2(glyph->imageWidth * _fontScale, (-glyph->top + glyph->imageHeight) * _fontScale);
		coords[3].vertex = Vector2(0, (-glyph->top + glyph->imageHeight) * _fontScale);

		coords[0].texcoord = Vector2(glyph->s, glyph->t);
		coords[1].texcoord = Vector2(glyph->s2, glyph->t);
		coords[2].texcoord = Vector2(glyph->s2, glyph->t2);
		coords[3].texcoord = Vector2(glyph->s, glyph->t2);
	}

	double getWidth() const
	{
		return glyph->xSkip * _fontScale;
	}

	// Offsets all coordinates by the given vector
	void offset(const Vector2& off)
	{
		coords[0].vertex += off;
		coords[1].vertex += off;
		coords[2].vertex += off;
		coords[3].vertex += off;
	}

	char getChar() const
	{
		return _char;
	}
};

class TextWord :
	public std::vector<TextChar>
{
public:
	double getWidth() const
	{
		double totalWidth = 0;

		for (TextWord::const_iterator i = begin(); i != end(); ++i)
		{
			totalWidth += i->getWidth();
		}

		return totalWidth;
	}

	void offset(const Vector2& off)
	{
		for (TextWord::iterator i = begin(); i != end(); ++i)
		{
			i->offset(off);
		}
	}

	// Named constructor, creates a word out of the given string for the given glyphset
	// The word is positioned at 0,0 after construction.
	static TextWord createForString(const std::string& string, fonts::IGlyphSet& glyphs, float fontScale)
	{
		TextWord result;
		Vector2 wordPos(0,0);

		for (std::string::const_iterator i = string.begin(); i != string.end(); ++i)
		{
			result.push_back(TextChar(*i, glyphs, fontScale));

			// Move character to position in word
			result.back().offset(wordPos);

			// Increase position pointer
			wordPos.x() += result.back().getWidth();
		}

		return result;
	}
};

class TextLine
{
public:
	typedef std::vector<TextChar> Chars;

private:
	// The width of the current line
	const double _lineWidth;

	Chars _chars;

	// The total width of all characters
	double _charWidth;

	// The font scale of this line
	float _fontScale;

public:
	TextLine(double width, float fontScale) :
		_lineWidth(width),
		_charWidth(0),
		_fontScale(fontScale)
	{}

	bool empty() const
	{
		return _chars.empty();
	}

	const Chars& getChars() const
	{
		return _chars;
	}

	double getWidth() const
	{
		return _charWidth;
	}

	// Add a word to this line
	// returns TRUE on success, FALSE if the word is too wide to fit in this line
	bool addWord(const std::string& word, fonts::IGlyphSet& glyphs, bool noclip = false)
	{
		// Generate the word
		TextWord tw = TextWord::createForString(word, glyphs, _fontScale);
		return addWord(tw, noclip);
	}

	// Adds a single character to this sentence
	// returns TRUE if the character fits, FALSE otherwise (character not added on FALSE).
	// Override the noclip argument with true to ignore width checks
	bool addChar(const char c, fonts::IGlyphSet& glyphs, bool noclip = false)
	{
		// Generate the character
		TextChar tc(c, glyphs, _fontScale);
		return addChar(tc, noclip);
	}

	// Offsets all character coordinates by the given vector
	void offset(const Vector2& off)
	{
		for (Chars::iterator i = _chars.begin(); i != _chars.end(); ++i)
		{
			i->offset(off);
		}
	}

	// If the line ends with a single space, this method will remove it
	void removeTrailingSpace()
	{
		if (_chars.empty()) return;

		if (_chars.rbegin()->getChar() == ' ')
		{
			_charWidth -= _chars.rbegin()->getWidth();
			_chars.pop_back();
		}
	}

private:
	bool addChar(TextChar& ch, bool noclip)
	{
		// Check the word length
		double remainingWidth = _lineWidth - _charWidth;
		double charWidth = ch.getWidth();

		if (!noclip && charWidth > remainingWidth + 1e-3)
		{
			return false;
		}

		// Move the word to the current position
		ch.offset(Vector2(_charWidth, 0));

		// Append the characters to our own vector
		_chars.push_back(ch);

		// Increase horizontal position
		_charWidth += charWidth;

		return true;
	}

	bool addWord(TextWord& word, bool noclip)
	{
		// Check the word length
		double remainingWidth = _lineWidth - _charWidth;
		double wordWidth = word.getWidth();

		// stgatilov: bug compatibility against Doom 3 engine
		// it ignores the last character when checking whether a word fits the line
		// see also: #5914 and https://forums.thedarkmod.com/index.php?/topic/21710-implicit-linebreaks-in-text/
		double wordWidthFit = wordWidth;
		if (!word.empty())
		{
			wordWidthFit -= word.back().getWidth();
		}

		if (!noclip && wordWidthFit > remainingWidth + 1e-3)
		{
			return false;
		}

		// Move the word to the current position
		word.offset(Vector2(_charWidth, 0));

		// Append the characters to our own vector
		_chars.insert(_chars.end(), word.begin(), word.end());

		// Increase horizontal position
		_charWidth += wordWidth;

		return true;
	}
};
typedef std::shared_ptr<TextLine> TextLinePtr;

} // namespace

#endif // TextParts_h__
