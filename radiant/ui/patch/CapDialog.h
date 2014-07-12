#pragma once

#include "wxutil/dialog/Dialog.h"
#include <map>
#include "patch/PatchConstants.h"

class wxRadioButton;
class wxFlexGridSizer;

/**
 * Query the user which type of cap should be created
 */
namespace ui
{

class PatchCapDialog :
	public wxutil::Dialog
{
private:
	EPatchCap _selectedCapType;

	typedef std::map<EPatchCap, wxRadioButton*> RadioButtons;
	RadioButtons _radioButtons;

public:
	// Constructor
	PatchCapDialog();

	// Returns the selected cap type (only valid if dialog result == OK)
	EPatchCap getSelectedCapType();

private:
	void addItemToTable(wxFlexGridSizer* sizer, const std::string& image, EPatchCap type);
};

} // namespace ui
