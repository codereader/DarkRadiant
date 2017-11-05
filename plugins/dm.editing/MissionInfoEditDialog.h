#pragma once

#include "icommandsystem.h"

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/XmlResourceBasedWidget.h"
#include "wxutil/TreeModel.h"

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

	// Treemodel definition
	struct MissionTitleColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		MissionTitleColumns() :
			number(add(wxutil::TreeModel::Column::Integer)),
			title(add(wxutil::TreeModel::Column::String))
		{}

		wxutil::TreeModel::Column number;
		wxutil::TreeModel::Column title;
	};

	MissionTitleColumns _missionTitleColumns;
	wxutil::TreeModel::Ptr _missionTitleStore;

public:
	// Constructor
	MissionInfoEditDialog(wxWindow* parent = nullptr);

	static void ShowDialog(const cmd::ArgumentList& args);

private:
	void populateWindow();
	void updateValuesFromDarkmodTxt();

	void onSave(wxCommandEvent& ev);
	void onCancel(wxCommandEvent& ev);
	void onTitleEdited(wxDataViewEvent& ev);
};

}