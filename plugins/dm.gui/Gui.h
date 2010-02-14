#ifndef Gui_h__
#define Gui_h__

#include <boost/shared_ptr.hpp>

namespace gui
{

/**
 * greebo: This class represents a single D3 GUI. It holds all
 * the windowDefs and the source code behind.
 */
class Gui
{
public:
	Gui();
};
typedef boost::shared_ptr<Gui> GuiPtr;

} // namespace

#endif // Gui_h__
