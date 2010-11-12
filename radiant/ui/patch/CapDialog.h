#ifndef _UI_PATCH_CAP_DIALOG_H_
#define _UI_PATCH_CAP_DIALOG_H_

#include "gtkutil/dialog/Dialog.h"
#include <map>
#include "patch/PatchConstants.h"

#include <gtkmm/radiobuttongroup.h>

namespace Gtk
{
	class Table;
	class RadioButton;
}

/**
 * Query the user which type of cap should be created
 */
namespace ui
{

class PatchCapDialog :
	public gtkutil::Dialog
{
private:
	EPatchCap _selectedCapType;

	typedef std::map<EPatchCap, Gtk::RadioButton*> RadioButtons;
	RadioButtons _radioButtons;

	Gtk::RadioButtonGroup _group;

public:
	// Constructor
	PatchCapDialog();

	// Returns the selected cap type (only valid if dialog result == OK)
	EPatchCap getSelectedCapType();

private:
	void addItemToTable(Gtk::Table& table, const std::string& image, int row, EPatchCap type);
};

} // namespace ui

#endif /* _UI_PATCH_CAP_DIALOG_H_ */
