#pragma once

#include "wxutil/dialog/Dialog.h"
#include <map>
#include "ipatch.h"
#include "icommandsystem.h"

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
	typedef std::map<patch::CapType, wxRadioButton*> RadioButtons;
	RadioButtons _radioButtons;

public:
	// Constructor
	PatchCapDialog();

	// Returns the selected cap type (only valid if dialog result == OK)
	patch::CapType getSelectedCapType();

	static void Show(const cmd::ArgumentList& args);

private:
	std::string getSelectedCapTypeString();

	void addItemToTable(wxFlexGridSizer* sizer, const std::string& image, patch::CapType type);
};

} // namespace ui
