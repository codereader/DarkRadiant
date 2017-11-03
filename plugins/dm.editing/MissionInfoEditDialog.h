#pragma once

#include "icommandsystem.h"

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/XmlResourceBasedWidget.h"

namespace ui
{

class MissionInfoEditDialog :
	public wxutil::DialogBase,
	private wxutil::XmlResourceBasedWidget
{
public:
	// Constructor
	MissionInfoEditDialog(wxWindow* parent = nullptr);

	static void ShowDialog(const cmd::ArgumentList& args);

protected:
	bool _onDeleteEvent() override;

private:
	void populateWindow();

	void onSave(wxCommandEvent& ev);
	void onCancel(wxCommandEvent& ev);
	void onFormatSelection(wxCommandEvent& ev);

	void saveOptionsToRegistry();
	void handleFormatSelectionChange();
};

}