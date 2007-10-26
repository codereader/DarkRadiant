#ifndef ECLASSTREE_H_
#define ECLASSTREE_H_

#include "iradiant.h"
#include "gtkutil/window/BlockingTransientWindow.h"
#include <boost/shared_ptr.hpp>

namespace ui {

class EClassTree;
typedef boost::shared_ptr<EClassTree> EClassTreePtr;

class EClassTree :
	public RadiantEventListener,
	public gtkutil::BlockingTransientWindow
{

	// Constructor, traverses the entity classes
	EClassTree();

	/** greebo: Accessor method for the singleton instance
	 */
	static EClassTree& Instance();
	
	// This is where the singleton instance shared_ptr is held 
	static EClassTreePtr& InstancePtr();

public:
	// Toggles the visibility of the singleton class (static command target)
	static void toggle();
	
	/** greebo: This toggles the visibility of the EClassTree dialog.
	 * The dialog is constructed only once and never destructed 
	 * during runtime.
	 */
	void toggleWindow();
};

} // namespace ui

#endif /*ECLASSTREE_H_*/

