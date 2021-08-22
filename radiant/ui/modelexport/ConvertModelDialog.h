#pragma once

#include "icommandsystem.h"

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/dataview/KeyValueTable.h"
#include "wxutil/XmlResourceBasedWidget.h"

namespace ui
{

class ConvertModelDialog :
	public wxutil::DialogBase,
	private wxutil::XmlResourceBasedWidget
{
private:
    wxutil::KeyValueTable* _infoTable;

public:
	// Constructor
    ConvertModelDialog(wxWindow* parent = nullptr);

	static void ShowDialog(const cmd::ArgumentList& args);

protected:
	bool _onDeleteEvent() override;

private:
	void populateWindow();

	void onConvert(wxCommandEvent& ev);
	void onCancel(wxCommandEvent& ev);
	void onInputPathChanged(wxCommandEvent& ev);
	void onFormatSelection(wxCommandEvent& ev);

	void saveOptionsToRegistry();
	void handleFormatSelectionChange();
	void handleInputPathChanged();
};

}
