#pragma once

#include <string>

namespace ui
{

class IAnimationChooser
{
public:
	virtual ~IAnimationChooser() {}

	struct Result
	{
		// The selected model
		std::string model;

		// The selected animation
		std::string anim;

		bool cancelled()
		{
			return model.empty() && anim.empty();
		}
	};

	// Run the dialog and return the selected model/anim combination. 
	// The dialog will enter a new event loop and block the UI until it's closed again.
	// The returned Result will be empty if the user clicks cancel.
	virtual Result runDialog(const std::string& preselectModel = std::string(),
		const std::string& preselectAnim = std::string()) = 0;

	// Destroys the window. Don't rely on the destructor, it won't call Destroy() for you.
	virtual void destroyDialog() = 0;
};

}
