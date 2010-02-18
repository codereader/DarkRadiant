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
	typedef std::vector<GuiWindowDefPtr> WindowDefs;
	WindowDefs _windows;

public:
	Gui();

	void addWindow(const GuiWindowDefPtr& window);

	// Takes the given token stream and attempts to construct a GUI object from it
	// Returns NULL on failure
	static GuiPtr createFromTokens(parser::DefTokeniser& tokeniser);
};

} // namespace

#endif // Gui_h__
