#ifndef GuiWindowDef_h__
#define GuiWindowDef_h__

#include "irender.h"
#include "math/Vector4.h"

namespace parser { class DefTokeniser; }

namespace gui
{

class GuiWindowDef;
typedef boost::shared_ptr<GuiWindowDef> GuiWindowDefPtr;

/**
 * greebo: This is the base class for all windowDef-like objects in a GUI,
 * including windowDef, choiceDef, bindDef, etc.
 */
class GuiWindowDef
{
public:
	// Public properties

	// The name of this windowDef
	std::string name;

	// Window size (x,y,width,height)
	Vector4 rect;

	// Visible or hidden
	bool visible;

	// Whether this gui is full screen (use on desktop window)
	bool menugui;

	Vector4 forecolor;
	Vector4 hovercolor;
	Vector4 backcolor;
	Vector4 bordercolor;
	Vector4 matcolor;

	float rotate;

	// background shader name
	std::string background;

	// background shader (NULL until realised)
	ShaderPtr backgroundShader;

	// The text to be rendered in this window
	std::string text;

	// The name of the font
	std::string font;

	// The scale for rendering the font
	float textscale;

	// The text alignment (left, right, center)
	int textalign;

	// No mouse events for this window
	bool noevents;

	// Whether this window forces text to wrap at their borders
	bool noclip;

	// Whether time is running for this windowDef
	bool notime;

	// The window time (0..infinity)
	std::size_t time;

	// All child windowDefs of this window
	typedef std::vector<GuiWindowDefPtr> ChildWindows;
	ChildWindows _children;

	// Default constructor
	GuiWindowDef() :
		visible(true),
		forecolor(1,1,1,1),
		hovercolor(1,1,1,1),
		backcolor(0,0,0,0),
		bordercolor(0,0,0,0),
		matcolor(1,1,1,1),
		rotate(0),
		textscale(1),
		textalign(0),
		noclip(false),
		notime(false),
		time(0)
	{}

	void constructFromTokens(parser::DefTokeniser& tokeniser);

	void addWindow(const GuiWindowDefPtr& window);

private:
	Vector4 parseVector4(parser::DefTokeniser& tokeniser);
	float parseFloat(parser::DefTokeniser& tokeniser);
	int parseInt(parser::DefTokeniser& tokeniser);
	std::string parseString(parser::DefTokeniser& tokeniser);
	bool parseBool(parser::DefTokeniser& tokeniser);
};


} // namespace

#endif // GuiWindowDef_h__
