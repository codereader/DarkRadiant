#pragma once

#include <vector>
#include <string>
#include <memory>
#include <sigc++/signal.h>
#include "imodule.h"
#include "math/Vector4.h"
#include "ishaders.h"

namespace gui
{

class IGui;

class IRenderableText
{
public:
	virtual ~IRenderableText() {}

	// Submits the openGL calls
	virtual void render() = 0;

	// Re-construct this structure, called when the text in the owning windowDef has been changed
	virtual void recompile() = 0;
};

// An expression as encountered in the GUI code
// Evaluates to an instance of a certain type (bool, string, etc.)
template<typename ValueType>
class IGuiExpression
{
public:
	virtual ~IGuiExpression() {}

	// Evaluate this expression to retrieve the result
	virtual ValueType evaluate() = 0;
};

// An expression representing a constant value
template<typename ValueType>
class ConstantExpression :
	public IGuiExpression<ValueType>
{
private:
	ValueType _value;

public:
	ConstantExpression(const ValueType& value) :
		_value(value)
	{}

	virtual ValueType evaluate() override
	{
		return _value;
	}

	template<typename OtherType>
	static std::shared_ptr<ConstantExpression<ValueType>> Create(const OtherType& value)
	{
		return std::make_shared<ConstantExpression<ValueType>>(value);
	}
};

// Represents a variable or property of a GuiWindowDef
// e.g. "text", "notime", "forecolor" or a user-defined variable
class IWindowVariable
{
private:
	sigc::signal<void> _changedSignal;

public:
	virtual ~IWindowVariable() {}

	// value-changed signal, to get notified when this value changes
	sigc::signal<void>& signal_variableChanged()
	{
		return _changedSignal;
	}
};

// Represents a GUI property carrying a scalar value
// e.g. "text" (std::string) or "textscale" (float)
template<typename ValueType>
class ScalarWindowVariable : 
	public IWindowVariable
{
protected:
	// Types used by this window variable
	typedef IGuiExpression<ValueType> ExpressionType;
	typedef std::shared_ptr<ExpressionType> ExpressionTypePtr;

	// The expression which can be evaluated
	ExpressionTypePtr _expression;

public:
	operator ValueType() const
	{
		return getValue();
	}

	virtual ValueType getValue() const
	{
		return _expression ? _expression->evaluate() : ValueType();
	}

	virtual void setValue(const ExpressionTypePtr& newExpr)
	{
		_expression = newExpr;
		signal_variableChanged().emit();
	}

	virtual void setValue(const ValueType& constantValue)
	{
		_expression = ConstantExpression<ValueType>::Create(constantValue);
		signal_variableChanged().emit();
	}
};

class IGuiWindowDef;
typedef std::shared_ptr<IGuiWindowDef> IGuiWindowDefPtr;

class IGuiWindowDef
{
public:
	// Public properties

	// The name of this windowDef
	std::string name;

	// Window size (x,y,width,height)
	Vector4 rect;

	// Visible or hidden
	ScalarWindowVariable<bool> visible;

	// The body text of this window
	ScalarWindowVariable<std::string> text;

	// Whether this gui is full screen (use on desktop window)
	ScalarWindowVariable<bool> menugui;

	Vector4 forecolor;
	Vector4 hovercolor;
	Vector4 backcolor;
	Vector4 bordercolor;
	ScalarWindowVariable<float> bordersize;
	Vector4 matcolor;

	ScalarWindowVariable<float> rotate;

	// background shader name
	ScalarWindowVariable<std::string> background;

	// background shader (NULL until realised)
	MaterialPtr backgroundShader;

	// The name of the font
	ScalarWindowVariable<std::string> font;

	// The scale for rendering the font
	ScalarWindowVariable<float> textscale;

	// The text alignment (left, right, center)
	ScalarWindowVariable<int> textalign;

	// Text offsets
	ScalarWindowVariable<float> textalignx;
	ScalarWindowVariable<float> textaligny;

	// Force a specific aspect ratio
	ScalarWindowVariable<float> forceaspectwidth;
	ScalarWindowVariable<float> forceaspectheight;

	// No mouse events for this window
	ScalarWindowVariable<bool> noevents;

	// Whether this window forces text to wrap at their borders
	ScalarWindowVariable<bool> noclip;

	// Whether time is running for this windowDef
	ScalarWindowVariable<bool> notime;

	// Don't display the cursor
	ScalarWindowVariable<bool> nocursor;

	// Don't wrap words at rectangle borders
	ScalarWindowVariable<bool> nowrap;

	// The window time (0..infinity)
	std::size_t time;

	// All child windowDefs of this window
	typedef std::vector<IGuiWindowDefPtr> ChildWindows;
	ChildWindows children;

public:
	virtual ~IGuiWindowDef() {}

	// Returns the owning GUI
	virtual IGui& getGui() const = 0;

	virtual void addWindow(const IGuiWindowDefPtr& window) = 0;

	// Recursively looks for a named child windowDef
	// Returns NULL if not found
	virtual IGuiWindowDefPtr findWindowDef(const std::string& name) = 0;

	// Get the renderable text object containing the OpenGLRenderables
	virtual IRenderableText& getRenderableText() = 0;

	/**
	* greebo: This is some sort of "think" method, giving this windowDef
	* a chance to handle timed events.
	*
	* @updateChildren: recursively updates child windowDef if true
	*/
	virtual void update(const std::size_t timeStep, bool updateChildren = true) = 0;

	// Initialises the time of this windowDef and all children
	virtual void initTime(const std::size_t time, bool updateChildren = true) = 0;

	// Prepares renderable objects, to be called by the parent Gui only
	virtual void pepareRendering(bool prepareChildren = true) = 0;
};

/**
* greebo: This class represents a single D3 GUI. It holds all
* the windowDefs and the source code behind.
*/
class IGui
{
public:
	virtual ~IGui() {}

	virtual const IGuiWindowDefPtr& getDesktop() const = 0;
	virtual void setDesktop(const IGuiWindowDefPtr& newDesktop) = 0;

	// Sets the given state variable (gui::<key> = <value>)
	virtual void setStateString(const std::string& key, const std::string& value) = 0;

	// Returns the state string "gui::<key>" or an empty string if non-existent
	virtual std::string getStateString(const std::string& key) = 0;

	// Sets up the time of the entire GUI (all windowDefs)
	virtual void initTime(const std::size_t time) = 0;

	// "Think" routine, advances all active windowDefs (where notime == false)
	virtual void update(const std::size_t timestep) = 0;

	// Returns a reference to the named windowDef, returns NULL if not found
	virtual IGuiWindowDefPtr findWindowDef(const std::string& name) = 0;

	// Called by the GuiRenderer to re-compile text VBOs, etc.
	virtual void pepareRendering() = 0;
};
typedef std::shared_ptr<IGui> IGuiPtr;

enum GuiType
{
	NOT_LOADED_YET,		// no attempt to load the GUI has been made
	UNDETERMINED,		// not checked yet for type
	ONE_SIDED_READABLE,	// 1-sided
	TWO_SIDED_READABLE,	// 2-sided
	NO_READABLE,		// not a readable
	IMPORT_FAILURE,		// failed to load
	FILE_NOT_FOUND,		// file doesn't exist
};

/**
* greebo: Interface managing the idTech4 GUI files,
* keeping track of all the loaded GUIs,
* parsing the .gui files on demand.
*/
class IGuiManager :
	public RegisterableModule
{
public:
	typedef std::vector<std::string> StringList;

	// A visitor class used to traverse all known GUIs by path
	class Visitor
	{
	public:
		virtual ~Visitor() {}

		virtual void visit(const std::string& guiPath, const GuiType& guiType) = 0;
	};

public:
	virtual ~IGuiManager() {}

	// Gets a GUI from the given VFS path, parsing it on demand
	// Returns NULL if the GUI couldn't be found or loaded.
	virtual IGuiPtr getGui(const std::string& guiPath) = 0;

	// Returns the number of known GUIs (or GUI paths)
	virtual std::size_t getNumGuis() = 0;

	// Traverse all known GUIs using the given Visitor
	virtual void foreachGui(Visitor& visitor) = 0;

	// Returns the GUI appearance type for the given GUI path
	virtual GuiType getGuiType(const std::string& guiPath) = 0;

	// Reload the gui
	virtual void reloadGui(const std::string& guiPath) = 0;

	// Returns the _errorList for use in a GUI.
	virtual const StringList& getErrorList() = 0;

	// Clears out the GUIs and reloads them
	virtual void reloadGuis() = 0;
};

}

const char* const MODULE_GUIMANAGER("GuiManager");

// Application-wide Accessor to the global GUI manager
inline gui::IGuiManager& GlobalGuiManager()
{
	// Cache the reference locally
	static gui::IGuiManager& _manager(
		*std::static_pointer_cast<gui::IGuiManager>(
			module::GlobalModuleRegistry().getModule(MODULE_GUIMANAGER))
	);
	return _manager;
}
