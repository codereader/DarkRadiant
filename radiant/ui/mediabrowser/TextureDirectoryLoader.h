#pragma once

#include "ishaders.h"

#include "wxutil/ModalProgressDialog.h"
#include "EventRateLimiter.h"

#include "i18n.h"
#include "string/predicate.h"
#include "../texturebrowser/TextureBrowserPanel.h"
#include "../texturebrowser/TextureBrowserManager.h"

namespace ui
{

/** Functor object to load all of the textures under the given directory
 * path. Loaded textures will then display in the Textures view.
 */

class TextureDirectoryLoader
{
	// Directory to search
	const std::string _searchDir;

	// Modal dialog window to display progress
	wxutil::ModalProgressDialog _dialog;

	// Event limiter for dialog updates
	EventRateLimiter _evLimiter;

public:
	// Constructor sets the directory to search
	TextureDirectoryLoader(const std::string& directory)
	: _searchDir(directory + "/"),
	  _dialog(_("Loading textures")),
     _evLimiter(100)
	{}

	// Functor operator
	void visit(const std::string& shaderName)
	{
		// Visited texture must start with the directory name
		// separated by a slash.
		if (string::istarts_with(shaderName, _searchDir))
		{
			// Update the text in the dialog
			if (_evLimiter.readyForEvent())
			{
				_dialog.setText(shaderName);
			}

			// Load the shader
			MaterialPtr ref = GlobalMaterialManager().getMaterial(shaderName);

            // Signal the texture browser that is has to update it next time it renders
            GlobalTextureBrowser().updateAllWindows();
		}
	}

};

}
