#pragma once

#include "igui.h"
#include "ishaders.h"
#include "math/Vector4.h"

#include "RenderableText.h"

namespace parser { class DefTokeniser; }

namespace gui
{

class Gui;

class GuiWindowDef;
typedef std::shared_ptr<GuiWindowDef> GuiWindowDefPtr;

class GuiScript;
typedef std::shared_ptr<GuiScript> GuiScriptPtr;

/**
 * greebo: This is the base class for all windowDef-like objects in a GUI,
 * including windowDef, choiceDef, bindDef, etc.
 */
class GuiWindowDef :
	public IGuiWindowDef
{
private:
	// The owning GUI
	IGui& _owner;

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
	// Default constructor
	GuiWindowDef(IGui& owner);

	// Returns the GUI
	IGui& getGui() const override;

	void constructFromTokens(parser::DefTokeniser& tokeniser);

	void addWindow(const IGuiWindowDefPtr& window) override;

	// Recursively looks for a named child windowDef
	// Returns NULL if not found
	IGuiWindowDefPtr findWindowDef(const std::string& name) override;

	const std::string& getText() const override;
	void setText(const std::string& newText) override;

	// Get the renderable text object containing the OpenGLRenderables
	IRenderableText& getRenderableText() override;

	/**
	 * greebo: This is some sort of "think" method, giving this windowDef
	 * a chance to handle timed events.
	 *
	 * @updateChildren: recursively updates child windowDef if true
	 */
	void update(const std::size_t timeStep, bool updateChildren = true) override;

	// Initialises the time of this windowDef and all children
	void initTime(const std::size_t time, bool updateChildren = true) override;

	// Prepares renderable objects, to be called by the parent Gui only
	void pepareRendering(bool prepareChildren = true) override;

private:
	Vector4 parseVector4(parser::DefTokeniser& tokeniser);
	float parseFloat(parser::DefTokeniser& tokeniser);
	int parseInt(parser::DefTokeniser& tokeniser);
	std::string parseString(parser::DefTokeniser& tokeniser);
	bool parseBool(parser::DefTokeniser& tokeniser);

	std::string getExpression(parser::DefTokeniser& tokeniser);
};


} // namespace
