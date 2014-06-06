#include "GroupSpecifierPanel.h"

namespace objectives
{

namespace ce
{

// Reg helper
GroupSpecifierPanel::RegHelper GroupSpecifierPanel::_regHelper;

GroupSpecifierPanel::GroupSpecifierPanel(wxWindow* parent) :
	TextSpecifierPanel(parent)
{
	// Set up the auto-completion
	wxArrayString choices;

	choices.Add("loot_total");
	choices.Add("loot_gold");
	choices.Add("loot_jewels");
	choices.Add("loot_goods");

	_entry->AutoComplete(choices);
}

} // namespace ce

} // namespace objectives
