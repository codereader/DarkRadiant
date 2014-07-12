#pragma once

#include "wxutil/dialog/Dialog.h"
#include "wxutil/XmlResourceBasedWidget.h"

/**
 * greebo: Dialog to query the user for the desired patch thickness and
 * if seams are to be created.
 */
namespace ui
{

class PatchThickenDialog :
	public wxutil::Dialog,
	private wxutil::XmlResourceBasedWidget
{
public:
	// Constructor, instantiate this class by specifying the parent window
	PatchThickenDialog();

	// Retrieve the user selection, use these after run() returned RESULT_OK
	float getThickness();
	bool getCeateSeams();
	int getAxis();
};

} // namespace
