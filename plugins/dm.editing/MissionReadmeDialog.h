#pragma once

#include "icommandsystem.h"

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/XmlResourceBasedWidget.h"

#include "MissionInfoGuiView.h"
#include "ReadmeTxt.h"

namespace wxutil { class GuiView; }

namespace ui
{

class MissionReadmeDialog :
	public wxutil::DialogBase,
	private wxutil::XmlResourceBasedWidget
{
private:
	MissionInfoGuiView* _guiView;

	map::ReadmeTxtPtr _readmeFile;

	bool _updateInProgress;

public:
	// Constructor
	MissionReadmeDialog(wxWindow* parent = nullptr);

	static void ShowDialog(const cmd::ArgumentList& args);

private:
	void populateWindow();
	void updateValuesFromReadmeFile();
	void setupNamedEntryBox(const std::string& ctrlName);

	void onSave(wxCommandEvent& ev);
	void onCancel(wxCommandEvent& ev);
};

}
