#pragma once

#include "icommandsystem.h"

#include "ModelExportOptions.h"
#include "wxutil/dialog/DialogBase.h"
#include "wxutil/XmlResourceBasedWidget.h"

namespace ui
{

class ExportAsModelDialog :
	public wxutil::DialogBase,
	private wxutil::XmlResourceBasedWidget
{
public:
	ExportAsModelDialog(wxWindow* parent = nullptr);

	static void ShowDialog(const cmd::ArgumentList& args);

protected:
	bool _onDeleteEvent() override;

private:
	void populateWindow();

	void onExport(wxCommandEvent& ev);
	void onCancel(wxCommandEvent& ev);
	void onFormatSelection(wxCommandEvent& ev);

	void saveOptionsToRegistry();
	void handleFormatSelectionChange();

    Vector3 getExportOrigin();
    model::ModelExportOrigin getSelectedExportOrigin();
    void setSelectedExportOrigin(model::ModelExportOrigin exportOrigin);
};

}
