#pragma once

#include "wxutil/dialog/Dialog.h"
#include "wxutil/XmlResourceBasedWidget.h"

/**
 * greebo: Dialog to query the user for the desired patch dimensions and
 * whether the selected brushes are to be removed after creation.
 */
namespace ui
{

class PatchCreateDialog :
	public wxutil::Dialog,
	private wxutil::XmlResourceBasedWidget
{
protected:
	void construct();

public:
	// Constructor
	PatchCreateDialog();

	// Get the selected values, use these after calling run()
	int getSelectedWidth();
	int getSelectedHeight();
	bool getRemoveSelectedBrush();
};

} // namespace ui
