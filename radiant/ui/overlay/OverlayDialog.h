#pragma once

#include "icommandsystem.h"

#include "gtkutil/window/TransientWindow.h"
#include "gtkutil/XmlResourceBasedWidget.h"

#include <map>
#include <string>

class wxFileDirPickerEvent;

namespace ui
{

class OverlayDialog;
typedef boost::shared_ptr<OverlayDialog> OverlayDialogPtr;

/**
 * Dialog to configure the background image overlay options for the ortho
 * window.
 */
class OverlayDialog :
	public wxutil::TransientWindow,
	private wxutil::XmlResourceBasedWidget
{
	Gtk::FileChooserButton* _fileChooserBtn;
    Gtk::Table* _subTable;
    Gtk::ToggleButton* _useImageBtn;

	// TRUE, if a widget update is in progress (to avoid callback loops)
	bool _callbackActive;

private:
	// Constructor sets up widgets
	OverlayDialog();

	// Widget construction helpers
	void setupDialog();

	void initialiseWidgets();
	void updateSensitivity();

	// callbacks
	void _onFileSelection(wxFileDirPickerEvent& ev);
	void _onToggleUseImage(wxCommandEvent& ev);
	void _onScrollChange();
	void _onClose(wxCommandEvent& ev);

	// Contains the pointer to the singleton instance
	static OverlayDialogPtr& InstancePtr();

	static OverlayDialog& Instance();

	void onRadiantShutdown();

	void _preShow();

public:
	/**
	 * Static method to display the overlay dialog.
	 */
	static void toggle(const cmd::ArgumentList& args);
};

}
