#pragma once

#include <string>
#include <memory>

class wxWindow;

namespace ui
{

/**
 * A Resource Chooser is a dialog for picking mod resources like
 * shaders, animations, sound shaders, particles, etc.
 *
 * The actual implementation varies a lot, but all resource pickers
 * share the ability to return the VFS path or name of the picked object
 * and to pre-select an item immediately after its shown.
 */
class IResourceChooser
{
public:
	virtual ~IResourceChooser() {}

	// Run the dialog and return the selected shader. The dialog will enter a 
	// new event loop and block the UI until it's closed again.
	// The returned string will be empty if the user clicks cancel.
	virtual std::string chooseResource(const std::string& preselected = std::string()) = 0;

	// Destroys the window. Don't rely on the destructor, it won't call Destroy() for you.
	virtual void destroyDialog() = 0;
};

}