#ifndef TEXTUREDIRECTORYLOADER_H_
#define TEXTUREDIRECTORYLOADER_H_

#include "imainframe.h"
#include "ishaders.h"

#include "gtkutil/ModalProgressDialog.h"
#include "EventRateLimiter.h"

#include "i18n.h"
#include <string>
#include <boost/algorithm/string/predicate.hpp>

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
	gtkutil::ModalProgressDialog _dialog;

   // Event limiter for dialog updates
   EventRateLimiter _evLimiter;

public:
	// Constructor sets the directory to search
	TextureDirectoryLoader(const std::string& directory)
	: _searchDir(directory + "/"),
	  _dialog(GlobalMainFrame().getTopLevelWindow(), _("Loading textures")),
     _evLimiter(100)
	{}

	// Functor operator
	void visit(const std::string& shaderName)
	{
		// Visited texture must start with the directory name
		// separated by a slash.
		if (boost::algorithm::istarts_with(shaderName, _searchDir))
		{
			// Update the text in the dialog
			if (_evLimiter.readyForEvent())
			{
				_dialog.setText("<b>" + shaderName + "</b>");
			}

			// Load the shader
			MaterialPtr ref = GlobalMaterialManager().getMaterialForName(shaderName);
		}
	}

};

}

#endif /*TEXTUREDIRECTORYLOADER_H_*/
