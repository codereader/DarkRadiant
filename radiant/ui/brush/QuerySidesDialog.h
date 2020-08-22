#pragma once

#include "icommandsystem.h"
#include "wxutil/dialog/DialogBase.h"

class wxSpinCtrl;

namespace ui
{

class QuerySidesDialog :
	public wxutil::DialogBase
{
private:
	// The spinner entry box
	wxSpinCtrl* _entry;

	// Number of selected sides
	int _numSides;

	// Min/max number of sides
	int _numSidesMin;
	int _numSidesMax;

	// Constructor
	QuerySidesDialog(int numSidesMin, int numSidesMax);

public:
	static void Show(const cmd::ArgumentList& args);

private:
	// This is called to initialise the dialog window / create the widgets
	void populateWindow();
};

} // namespace ui
