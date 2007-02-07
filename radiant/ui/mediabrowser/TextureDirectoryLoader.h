#ifndef TEXTUREDIRECTORYLOADER_H_
#define TEXTUREDIRECTORYLOADER_H_

#include "ishaders.h"

#include "gtkutil/ModalProgressDialog.h"
#include "mainframe.h"

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
	
public:
	typedef const char* first_argument_type;
	
	// Constructor sets the directory to search
	TextureDirectoryLoader(const std::string& directory)
	: _searchDir(directory + "/"),
	  _dialog(MainFrame_getWindow(), "Loading textures")
	{}
	
	// Functor operator
	void operator() (const char* shaderName) {

		const std::string sName(shaderName);

		// Visited texture must start with the directory name
		// separated by a slash.
		if (boost::algorithm::istarts_with(sName, _searchDir)) {
			// Update the text in the dialog
			_dialog.setText("<b>" + sName + "</b>");
			// Load the shader
			IShaderPtr ref = GlobalShaderSystem().getShaderForName(shaderName);
		}
	}
	
};

}

#endif /*TEXTUREDIRECTORYLOADER_H_*/
