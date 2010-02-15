#ifndef Gui_h__
#define Gui_h__

#include <boost/shared_ptr.hpp>
#include "parser/DefTokeniser.h"

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
public:
	Gui();

	// Takes the given token stream and attempts to construct a GUI object from it
	// Returns NULL on failure
	static GuiPtr createFromTokens(parser::DefTokeniser& tokeniser);
};

} // namespace

#endif // Gui_h__
