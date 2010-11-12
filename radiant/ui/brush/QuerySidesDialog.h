#ifndef _QUERY_SIDES_DIALOG_H_
#define _QUERY_SIDES_DIALOG_H_

#include "gtkutil/window/BlockingTransientWindow.h"

namespace Gtk
{
	class SpinButton;
	class Widget;
}

namespace ui
{

class QuerySidesDialog :
	public gtkutil::BlockingTransientWindow
{
public:
	enum Result
	{
		RESULT_CANCEL,
		RESULT_OK,
		NUM_RESULTS
	};

private:
	// The spinner entry box
	Gtk::SpinButton* _entry;

	Result _result;

	// Number of selected sides
	int _numSides;

	// Min/max number of sides
	int _numSidesMin;
	int _numSidesMax;

public:
	// Constructor
	QuerySidesDialog(int numSidesMin, int numSidesMax);

	/**
	 * greebo: Shows the dialog, returns the number of sides as selected by the user.
	 * Returns -1 on failure.
	 */
	int queryNumberOfSides();

private:
	// This is called to initialise the dialog window / create the widgets
	void populateWindow();
	Gtk::Widget& createButtons();

	// The callback for the buttons
	void onOK();
	void onCancel();
};

} // namespace ui

#endif /* _QUERY_SIDES_DIALOG_H_ */
