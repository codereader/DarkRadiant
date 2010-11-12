#ifndef PATCHTHICKENDIALOG_
#define PATCHTHICKENDIALOG_

#include "gtkutil/dialog/Dialog.h"
#include <gtkmm/radiobuttongroup.h>

namespace Gtk
{
	class CheckButton;
	class Entry;
	class RadioButton;
}

/**
 * greebo: Dialog to query the user for the desired patch thickness and
 * if seams are to be created.
 */
namespace ui
{

class PatchThickenDialog :
	public gtkutil::Dialog
{
private:
	Gtk::Entry* _thicknessEntry;
	Gtk::CheckButton* _seamsCheckBox;

	Gtk::RadioButtonGroup _group;

	Gtk::RadioButton* _radNormals;
	Gtk::RadioButton* _radX;
	Gtk::RadioButton* _radY;
	Gtk::RadioButton* _radZ;

public:
	// Constructor, instantiate this class by specifying the parent window
	PatchThickenDialog();

	// Retrieve the user selection, use these after run() returned RESULT_OK
	float getThickness();
	bool getCeateSeams();
	int getAxis();
};

} // namespace

#endif /*PATCHTHICKENDIALOG_*/
