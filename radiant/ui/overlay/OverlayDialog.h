#ifndef OVERLAYDIALOG_H_
#define OVERLAYDIALOG_H_

#include "icommandsystem.h"

#include "gtkutil/window/PersistentTransientWindow.h"

#include <gtkmm/filechooserbutton.h>
#include <gtkmm/table.h>
#include <gtkmm/togglebutton.h>

#include <map>
#include <string>

namespace Gtk
{
	class Widget;
}

namespace ui
{

class OverlayDialog;
typedef boost::shared_ptr<OverlayDialog> OverlayDialogPtr;

/**
 * Dialog to configure the background image overlay options for the ortho
 * window.
 */
class OverlayDialog :
	public gtkutil::PersistentTransientWindow
{
	Gtk::FileChooserButton* _fileChooserBtn;
    Gtk::Table* _subTable;
    Gtk::ToggleButton* _useImageBtn;

	// TRUE, if a widget update is in progress (to avoid callback loops)
	bool _callbackActive;

private:
	// Constructor creates GTK widgets
	OverlayDialog();

	// Widget construction helpers
	Gtk::Widget& createWidgets();
	Gtk::Widget& createButtons();

	void initialiseWidgets();
	void updateSensitivity();

	// gtkmm callbacks
	void _onClose();
	void _onFileSelection();
	void toggleUseImage();
	void _onScrollChange();

	// Contains the pointer to the singleton instance
	static OverlayDialogPtr& InstancePtr();

public:
	/**
	 * Static method to display the overlay dialog.
	 */
	static void display(const cmd::ArgumentList& args);

	// Called at shutdown to free the instance
	static void destroy();
};

}

#endif /*OVERLAYDIALOG_H_*/
