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
	Vector2 texcoords;
};

class TextChar
{
public:
	fonts::IGlyphInfoPtr glyph;

	// The 4 coordinates, marking the quad (3D and UV)
	Vertex2D coords[4];

	TextChar(unsigned char c, fonts::IGlyphSet& glyphs)
	{
		glyph = glyphs.getGlyph(static_cast<std::size_t>(c));

		coords[0].vertex = Vector2(0, 0);
		coords[1].vertex = Vector2(glyph->imageWidth, 0);
		coords[2].vertex = Vector2(glyph->imageWidth, glyph->imageHeight);
		coords[3].vertex = Vector2(0, glyph->imageHeight);

		coords[0].texcoords = Vector2(glyph->s, glyph->t);
		coords[1].texcoords = Vector2(glyph->s2, glyph->t);
		coords[2].texcoords = Vector2(glyph->s2, glyph->t2);
		coords[3].texcoords = Vector2(glyph->s, glyph->t2);
	}

	double getWidth() const
	{
		return coords[2].vertex.x() - coords[0].vertex.x();
	}

	// Offsets all coordinates by the given vector
	void offset(const Vector2& off)
	{
		coords[0].vertex += off;
		coords[1].vertex += off;
		coords[2].vertex += off;
		coords[3].vertex += off;
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
	static TextWord createForString(const std::string& string, fonts::IGlyphSet& glyphs)
	{
		TextWord result;
		Vector2 wordPos(0,0);

		for (std::string::const_iterator i = string.begin(); i != string.end(); ++i)
		{
			result.push_back(TextChar(*i, glyphs));

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
private:
	// The width of the current line
	const double _lineWidth;

	typedef std::vector<TextChar> Chars;
	Chars _chars;

	// The total width of all characters
	double _charWidth;

public:
	TextLine(double width) :
		_lineWidth(width),
		_charWidth(0)
	{}

	bool empty() const
	{
		return _chars.empty();
	}

	// Add a word to this line
	// returns TRUE on success, FALSE if the word is too wide to fit in this line
	bool addWord(const std::string& w, fonts::IGlyphSet& glyphs, bool noclip = false)
	{
		std::string word(w);

		// If we already have some characters, add a space
		word += (!_chars.empty()) ? " " : "";
		
		// Generate the word
		TextWord tw = TextWord::createForString(word, glyphs);
		return addWord(tw, noclip);
	}

	// Offsets all character coordinates by the given vector
	void offset(const Vector2& off)
	{
		for (Chars::iterator i = _chars.begin(); i != _chars.end(); ++i)
		{
			i->offset(off);
		}
	}

private:
	bool addWord(TextWord& word, bool noclip)
	{
		// Check the word length
		double remainingWidth = _lineWidth - _charWidth;
		double wordWidth = word.getWidth();

		if (!noclip && wordWidth > remainingWidth)
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
typedef boost::shared_ptr<TextLine> TextLinePtr;

} // namespace

#endif // TextParts_h__
