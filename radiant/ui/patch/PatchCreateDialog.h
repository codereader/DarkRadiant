#ifndef PATCHCREATEDIALOG_H_
#define PATCHCREATEDIALOG_H_

#include "gtkutil/dialog/Dialog.h"

namespace Gtk
{
	class ComboBoxText;
	class CheckButton;
}

/**
 * greebo: Dialog to query the user for the desired patch dimensions and
 * whether the selected brushes are to be removed after creation.
 */
namespace ui
{

class PatchCreateDialog :
	public gtkutil::Dialog
{
private:
	Gtk::ComboBoxText* _comboWidth;
	Gtk::ComboBoxText* _comboHeight;
	Gtk::CheckButton* _removeBrushCheckbox;

protected:
	void _postShow();

public:
	// Constructor
	PatchCreateDialog();

	// Get the selected values, use these after calling run()
	int getSelectedWidth();
	int getSelectedHeight();
	bool getRemoveSelectedBrush();
};

} // namespace ui

#endif /*PATCHCREATEDIALOG_H_*/
