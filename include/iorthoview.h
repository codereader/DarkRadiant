#ifndef _IORTHOVIEW_H_
#define _IORTHOVIEW_H_

#include "imodule.h"

class IXWndManager :
	public RegisterableModule
{
public:
	// Passes a queueDraw() call to each allocated view
	virtual void updateAllViews() = 0;
};

const char* const MODULE_ORTHOVIEWMANAGER = "OrthoviewManager";

// This is the accessor for the xy window manager module
inline IXWndManager& GlobalXYWndManager()
{
	// Cache the reference locally
	static IXWndManager& _xyWndManager(
		*boost::static_pointer_cast<IXWndManager>(
			module::GlobalModuleRegistry().getModule(MODULE_ORTHOVIEWMANAGER)
		)
	);
	return _xyWndManager;
}

#endif /* _IORTHOVIEW_H_ */
