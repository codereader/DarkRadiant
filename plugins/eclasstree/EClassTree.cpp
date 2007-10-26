#include "EClassTree.h"



namespace ui {

EClassTree::EClassTree() :
	gtkutil::BlockingTransientWindow("Entity Class Tree", GlobalRadiant().getMainWindow())
{
	// Construct the window's widgets
}

void EClassTree::toggleWindow() {
	if (isVisible()) {
		hide();
	}
	else {
		show();
	}
}

EClassTree& EClassTree::Instance() {
	return *InstancePtr();
}

// Static command target
void EClassTree::toggle() {
	Instance().toggleWindow();
}

EClassTreePtr& EClassTree::InstancePtr() {
	static EClassTreePtr _instancePtr;
		
	if (_instancePtr == NULL) {
		// Not yet instantiated, do it now
		_instancePtr = EClassTreePtr(new EClassTree);
		
		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().addEventListener(_instancePtr);
	}
	
	return _instancePtr;
}

} // namespace ui
