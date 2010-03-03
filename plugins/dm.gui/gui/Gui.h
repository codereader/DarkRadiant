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

/**
 * greebo: This class represents a single D3 GUI. It holds all
 * the windowDefs and the source code behind.
 */
class Gui
{
private:
	// The desktop window
	GuiWindowDefPtr _desktop;

public:
	Gui();

	const GuiWindowDefPtr& getDesktop() const;
	void setDesktop(const GuiWindowDefPtr& newDesktop);

	// Sets up the time of the entire GUI (all windowDefs)
	void initTime(const std::size_t time);

	// "Think" routine, advances all active windowDefs (where notime == false)
	void update(const std::size_t timestep);

	// Returns a reference to the named windowDef, returns NULL if not found
	GuiWindowDefPtr findWindowDef(const std::string& name);

	// Takes the given token stream and attempts to construct a GUI object from it
	// Returns NULL on failure
	static GuiPtr createFromTokens(parser::DefTokeniser& tokeniser);
};

} // namespace

#endif // Gui_h__
