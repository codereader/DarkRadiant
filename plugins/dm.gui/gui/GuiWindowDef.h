#ifndef GuiWindowDef_h__
#define GuiWindowDef_h__

#include "ishaders.h"
#include "math/Vector4.h"

#include "RenderableText.h"

namespace parser { class DefTokeniser; }

namespace gui
{

class Gui;

class GuiWindowDef;
typedef boost::shared_ptr<GuiWindowDef> GuiWindowDefPtr;

class GuiScript;
typedef boost::shared_ptr<GuiScript> GuiScriptPtr;

/**
 * greebo: This is the base class for all windowDef-like objects in a GUI,
 * including windowDef, choiceDef, bindDef, etc.
 */
class GuiWindowDef
{
private:
	// The owning GUI
	Gui& _owner;

	// The renderable text object for submission to a Renderer
	RenderableText _renderableText;

	// Whether the text has changed
	bool _textChanged;

	// The text to be rendered in this window (private, use getText() and setText())
	std::string _text;

	// The mapping between time and GUI scripts
	typedef std::multimap<std::size_t, GuiScriptPtr> TimedEventMap;
	TimedEventMap _timedEvents;

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
	MaterialPtr backgroundShader;

	// The name of the font
	std::string font;

	// The scale for rendering the font
	float textscale;

	// The text alignment (left, right, center)
	int textalign;

	// Text offsets
	float textalignx;
	float textaligny;

	// Force a specific aspect ratio
	float forceaspectwidth;
	float forceaspectheight;

	// No mouse events for this window
	bool noevents;

	// Whether this window forces text to wrap at their borders
	bool noclip;

	// Whether time is running for this windowDef
	bool notime;

	// Don't display the cursor
	bool nocursor;

	// Don't wrap words at rectangle borders
	bool nowrap;

	// The window time (0..infinity)
	std::size_t time;

	// All child windowDefs of this window
	typedef std::vector<GuiWindowDefPtr> ChildWindows;
	ChildWindows children;

public:
	// Default constructor
	GuiWindowDef(Gui& owner);

	// Returns the GUI
	Gui& getGui() const;

	void constructFromTokens(parser::DefTokeniser& tokeniser);

	void addWindow(const GuiWindowDefPtr& window);

	// Recursively looks for a named child windowDef
	// Returns NULL if not found
	GuiWindowDefPtr findWindowDef(const std::string& name);

	const std::string& getText() const;
	void setText(const std::string& newText);

	// Get the renderable text object containing the OpenGLRenderables
	RenderableText& getRenderableText();

	/**
	 * greebo: This is some sort of "think" method, giving this windowDef
	 * a chance to handle timed events.
	 *
	 * @updateChildren: recursively updates child windowDef if true
	 */
	void update(const std::size_t timeStep, bool updateChildren = true);

	// Initialises the time of this windowDef and all children
	void initTime(const std::size_t time, bool updateChildren = true);

	// Prepares renderable objects, to be called by the parent Gui only
	void pepareRendering(bool prepareChildren = true);

private:
	Vector4 parseVector4(parser::DefTokeniser& tokeniser);
	float parseFloat(parser::DefTokeniser& tokeniser);
	int parseInt(parser::DefTokeniser& tokeniser);
	std::string parseString(parser::DefTokeniser& tokeniser);
	bool parseBool(parser::DefTokeniser& tokeniser);

	std::string getExpression(parser::DefTokeniser& tokeniser);
};


} // namespace

#endif // GuiWindowDef_h__
