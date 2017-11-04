#pragma once

#include "icommandsystem.h"

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/XmlResourceBasedWidget.h"

#include "DarkmodTxt.h"

namespace ui
{

class MissionInfoEditDialog :
	public wxutil::DialogBase,
	private wxutil::XmlResourceBasedWidget
{
private:
	// The file we're editing
	map::DarkmodTxtPtr _darkmodTxt;

public:
	// Constructor
	MissionInfoEditDialog(wxWindow* parent = nullptr);

	static void ShowDialog(const cmd::ArgumentList& args);

private:
	void populateWindow();
	void updateValuesFromDarkmodTxt();

	void onSave(wxCommandEvent& ev);
	void onCancel(wxCommandEvent& ev);
};

}