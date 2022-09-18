#pragma once

#include <sigc++/connection.h>
#include "imodel.h"

#include "wxutil/dialog/DialogBase.h"
#include <string>

#include "wxutil/dataview/ResourceTreeView.h"

namespace ui
{

class SkinSelector;

/** Dialog to allow selection of skins for a model. Skins are grouped
 * into two toplevel categories - matching skins which are associated with the
 * model, and all skins available.
 */
class SkinChooser :
	public wxutil::DialogBase
{
private:
	// The model name to use for skin matching
	std::string _model;

    SkinSelector* _selector;

private:
	// Constructor creates widgets
	SkinChooser(const std::string& model);

	// Retrieve the currently selected skin
	std::string getSelectedSkin();
    void setSelectedSkin(const std::string& skin);

public:
	int ShowModal() override;

	/** Display the dialog and return the skin chosen by the user, or an empty
	 * string if no selection was made. This static method enters are recursive
	 * main loop during skin selection.
	 *
	 * @param model
	 * The full VFS path of the model for which matching skins should be found.
	 *
	 * @param prevSkin
	 * The current skin set on the model, so that the original can be returned
	 * if the dialog is cancelled.
	 */
	static std::string chooseSkin(const std::string& model,
								  const std::string& prevSkin);
};

}
