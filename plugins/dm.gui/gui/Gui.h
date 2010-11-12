#ifndef Gui_h__
#define Gui_h__

#include <map>
#include <boost/shared_ptr.hpp>
#include "parser/DefTokeniser.h"
#include "GuiWindowDef.h"

namespace gui
{

class Gui;
typedef boost::shared_ptr<Gui> GuiPtr;

class GuiWindowDef;
typedef boost::shared_ptr<GuiWindowDef> GuiWindowDefPtr;

/**
 * greebo: This class represents a single D3 GUI. It holds all
 * the windowDefs and the source code behind.
 */
class Gui
{
private:
	// The desktop window
	GuiWindowDefPtr _desktop;

	// The global GUI state variables
	typedef std::map<std::string, std::string> GuiState;
	GuiState _state;

public:
	Gui();

	const GuiWindowDefPtr& getDesktop() const;
	void setDesktop(const GuiWindowDefPtr& newDesktop);

	// Sets the given state variable (gui::<key> = <value>)
	void setStateString(const std::string& key, const std::string& value);

	// Returns the state string "gui::<key>" or an empty string if non-existent
	std::string getStateString(const std::string& key);

	// Sets up the time of the entire GUI (all windowDefs)
	void initTime(const std::size_t time);

	// "Think" routine, advances all active windowDefs (where notime == false)
	void update(const std::size_t timestep);

	// Returns a reference to the named windowDef, returns NULL if not found
	GuiWindowDefPtr findWindowDef(const std::string& name);

	// Called by the GuiRenderer to re-compile text VBOs, etc.
	void pepareRendering();

	// Takes the given token stream and attempts to construct a GUI object from it
	// Returns NULL on failure
	static GuiPtr createFromTokens(parser::DefTokeniser& tokeniser);
};

} // namespace

#endif // Gui_h__
