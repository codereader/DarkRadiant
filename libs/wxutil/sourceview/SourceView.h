#pragma once

#include <map>
#include <wx/stc/stc.h>

namespace wxutil
{

/**
 * greebo: This is a custom extension of the wxWidgets styles text control,
 * providing a few methods to make the actual code style mapping easier.
 * It's advisable to subclass this control and map the various lexer-recognised 
 * elements to a specific appearance.
 */
class SourceViewCtrl :
	public wxStyledTextCtrl
{
public:
	enum FontStyle
	{
		Normal = 1,
		Italic = 2,
		Bold = 4,
		Underline = 8,
		Hidden = 16,
	};

	// Describes a specific style (e.g. a code comment)
	struct Style
	{
		wxString foreground;
		wxString fontname;
		int fontsize;
		FontStyle fontstyle;

		Style() :
			foreground("BLACK"),
			fontname(""),
			fontsize(10),
			fontstyle(Normal)
		{};

		Style(const char* foreground_, 
				  FontStyle fontStyle_ = Normal,
				  int fontSize_ = 10,
				  const char* fontname_ = "") :
			foreground(foreground_),
			fontname(fontname_),
			fontsize(fontSize_),
			fontstyle(fontStyle_)
		{}
	};

	// Elements as recognised by the STC lexer
	enum Element
	{
		Default = 0,
		Keyword1,
		Keyword2,
		Keyword3,
		Keyword4,
		Keyword5,
		Keyword6,
		Comment,
		CommentDoc,
		CommentLine,
		SpecialComment,
		Character,
		CharacterEOL,
		String,
		StringEOL,
		Delimiter,
		Punctuation,
		Operator,
		Brace,
		Command,
		Identifier,
		Label,
		Number,
		Parameter,
		RegEx,
		UUID,
		Value,
		Preprocessor,
		Script,
		Error,
		Undefined,
		NumElements,
	};

protected:
	typedef std::map<Element, Style> StyleMap;
	StyleMap _predefinedStyles;

public:
	SourceViewCtrl(wxWindow* parent);

	virtual ~SourceViewCtrl() {}

	// Use this method to set a lexer-recognised element to a certain style
	// e.g. SetStyleMapping(0, Number), provided that the lexer is using 
	// the index 0 to represent numbers in the source.
	virtual void SetStyleMapping(int elementIndex, Element elementType);
};

/**
 * A special class providing syntax highlighting for the Python 
 * scripting language.
 */
class PythonSourceViewCtrl :
	public SourceViewCtrl
{
public:
	PythonSourceViewCtrl(wxWindow* parent);
};

/**
 * A base class providing highlighting for the Doom 3
 * declaration syntax, loosely based on C++ highlighting.
 */
class D3DeclarationViewCtrl :
	public SourceViewCtrl
{
public:
	D3DeclarationViewCtrl(wxWindow* parent);
};

/**
 * A special class providing highlighting for the Doom 3
 * material syntax.
 */
class D3MaterialSourceViewCtrl :
	public D3DeclarationViewCtrl
{
public:
	D3MaterialSourceViewCtrl(wxWindow* parent);
};

/**
 * A special class providing highlighting for the Doom 3
 * sound shader syntax.
 */
class D3SoundShaderSourceViewCtrl :
	public D3DeclarationViewCtrl
{
public:
	D3SoundShaderSourceViewCtrl(wxWindow* parent);
};

/**
 * A special class providing highlighting for the Doom 3
 * particle syntax.
 */
class D3ParticleSourceViewCtrl :
    public D3DeclarationViewCtrl
{
public:
    D3ParticleSourceViewCtrl(wxWindow* parent);
};

/**
* A special class providing highlighting for the Doom 3
* modelDef syntax.
*/
class D3ModelDefSourceViewCtrl :
    public D3DeclarationViewCtrl
{
public:
    D3ModelDefSourceViewCtrl(wxWindow* parent);
};

} // namespace
