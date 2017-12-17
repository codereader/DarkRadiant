#include "MissionInfoEditDialog.h"

#include "igui.h"
#include "itextstream.h"
#include "i18n.h"
#include <sigc++/functors/mem_fun.h>

#include <functional>
#include <fmt/format.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include "wxutil/TreeView.h"
#include "wxutil/preview/GuiView.h"
#include "wxutil/dialog/MessageBox.h"
#include "wxutil/menu/IconTextMenuItem.h"

#include "MissionInfoGuiView.h"
#include "MissionReadmeDialog.h"

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Mission Info Editor (darkmod.txt)");
}

MissionInfoEditDialog::MissionInfoEditDialog(wxWindow* parent) :
	DialogBase(_(WINDOW_TITLE), parent),
	_missionTitleStore(new wxutil::TreeModel(_missionTitleColumns, true)),
	_updateInProgress(false)
{
	populateWindow();

	try
	{
		_darkmodTxt = map::DarkmodTxt::LoadForCurrentMod();
	}
	catch (map::DarkmodTxt::ParseException& ex)
	{
		rError() << "Failed to parse darkmod.txt: " << ex.what() << std::endl;

		wxutil::Messagebox::ShowError(
			fmt::format(_("Failed to parse darkmod.txt:\n{0}"), ex.what()), this);

		// Reset the file to defaults
		_darkmodTxt = std::make_shared<map::DarkmodTxt>();
	}

	_guiView->setGui(GlobalGuiManager().getGui("guis/mainmenu.gui"));
	_guiView->setMissionInfoFile(_darkmodTxt);

	updateValuesFromDarkmodTxt();
}

void MissionInfoEditDialog::updateValuesFromDarkmodTxt()
{
	_missionTitleStore->Clear();

	assert(_darkmodTxt); // this should be non-NULL at all times

	if (!_darkmodTxt) return;

	_updateInProgress = true;

	findNamedObject<wxTextCtrl>(this, "MissionInfoEditDialogTitleEntry")->SetValue(_darkmodTxt->getTitle());
	findNamedObject<wxTextCtrl>(this, "MissionInfoEditDialogAuthorEntry")->SetValue(_darkmodTxt->getAuthor());
	findNamedObject<wxTextCtrl>(this, "MissionInfoEditDialogDescriptionEntry")->SetValue(_darkmodTxt->getDescription());
	findNamedObject<wxTextCtrl>(this, "MissionInfoEditDialogVersionEntry")->SetValue(_darkmodTxt->getVersion());
	findNamedObject<wxTextCtrl>(this, "MissionInfoEditDialogReqTdmVersionEntry")->SetValue(_darkmodTxt->getReqTdmVersion());
	findNamedObject<wxStaticText>(this, "MissionInfoEditDialogOutputPath")->SetLabelText(_darkmodTxt->getFullOutputPath());

	const map::DarkmodTxt::TitleList& titles = _darkmodTxt->getMissionTitles();

	// We skip the first entry, which is the campaign title
	for (std::size_t i = 1; i < titles.size(); ++i)
	{
		const std::string& title = titles[i];
		
		wxutil::TreeModel::Row row = _missionTitleStore->AddItem();

		row[_missionTitleColumns.number] = static_cast<int>(i);
		row[_missionTitleColumns.title] = title;

		row.SendItemAdded();
	}

	_guiView->update();

	_updateInProgress = false;
}

void MissionInfoEditDialog::populateWindow()
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxPanel* panel = loadNamedPanel(this, "MissionInfoEditDialogMainPanel");
	GetSizer()->Add(panel, 1, wxEXPAND);

	// Replace the list control with our own TreeView
	wxWindow* existing = findNamedObject<wxWindow>(this, "MissionInfoEditDialogMissionTitleList");

	wxutil::TreeView* treeview = wxutil::TreeView::CreateWithModel(existing->GetParent(), _missionTitleStore, wxDV_SINGLE);

	treeview->SetName("MissionInfoEditDialogMissionTitleList");
	treeview->SetMinSize(wxSize(-1, 150));

	// Display name column with icon
	treeview->AppendTextColumn(_("#"), _missionTitleColumns.number.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);

	treeview->AppendTextColumn(_("Title"), _missionTitleColumns.title.getColumnIndex(),
		wxDATAVIEW_CELL_EDITABLE, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);

	treeview->Connect(wxEVT_DATAVIEW_ITEM_EDITING_DONE,
		wxDataViewEventHandler(MissionInfoEditDialog::onTitleEdited), nullptr, this);
	treeview->Connect(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU,
		wxDataViewEventHandler(MissionInfoEditDialog::onTitleContextMenu), nullptr, this);

	existing->GetContainingSizer()->Replace(existing, treeview);
	existing->Destroy();

	// Add the preview widget
	wxPanel* previewPanel = findNamedObject<wxPanel>(this, "MissionInfoEditDialogPreviewPanel");
	_guiView = new MissionInfoGuiView(previewPanel);
	previewPanel->GetSizer()->Add(_guiView, 1, wxEXPAND);

	makeLabelBold(this, "MissionInfoLabel");

	wxButton* saveButton = findNamedObject<wxButton>(this, "MissionInfoEditDialogSaveButton");
	wxButton* cancelButton = findNamedObject<wxButton>(this, "MissionInfoEditDialogCancelButton");

	saveButton->Bind(wxEVT_BUTTON, sigc::mem_fun(this, &MissionInfoEditDialog::onSave));
	cancelButton->Bind(wxEVT_BUTTON, sigc::mem_fun(this, &MissionInfoEditDialog::onCancel));
	
	// Popup Menu
	_missionTitlesContextMenu.reset(new wxutil::PopupMenu);

	_missionTitlesContextMenu->addItem(
		new wxutil::StockIconTextMenuItem(_("Add Title"), wxART_PLUS),
		std::bind(&MissionInfoEditDialog::onAddTitle, this)
	);

	_missionTitlesContextMenu->addItem(
		new wxutil::StockIconTextMenuItem(_("Delete Title"), wxART_MINUS),
		std::bind(&MissionInfoEditDialog::onDeleteTitle, this),
		std::bind(&MissionInfoEditDialog::testDeleteTitle, this)
	);

	// Wire up the text entry boxes to update the preview
	setupNamedEntryBox("MissionInfoEditDialogTitleEntry");
	setupNamedEntryBox("MissionInfoEditDialogAuthorEntry");
	setupNamedEntryBox("MissionInfoEditDialogDescriptionEntry");
	setupNamedEntryBox("MissionInfoEditDialogVersionEntry");

	// Setup the event for the readme.txt button
	wxButton* editReadmeButton = findNamedObject<wxButton>(this, "MissionInfoEditDialogEditReadmeButton");
	editReadmeButton->Bind(wxEVT_BUTTON, sigc::mem_fun(this, &MissionInfoEditDialog::onEditReadme));

	Layout();
	Fit();
	CenterOnScreen();
}

void MissionInfoEditDialog::setupNamedEntryBox(const std::string& ctrlName)
{
	wxTextCtrl* ctrl = findNamedObject<wxTextCtrl>(this, ctrlName);

	assert(ctrl != nullptr);
	if (ctrl == nullptr) return;

	ctrl->Bind(wxEVT_TEXT, [this](wxCommandEvent& ev) 
	{
		if (_updateInProgress) return;

		// Load the values from the UI to the DarkmodTxt instance
		_darkmodTxt->setTitle(findNamedObject<wxTextCtrl>(this, "MissionInfoEditDialogTitleEntry")->GetValue().ToStdString());
		_darkmodTxt->setAuthor(findNamedObject<wxTextCtrl>(this, "MissionInfoEditDialogAuthorEntry")->GetValue().ToStdString());
		_darkmodTxt->setDescription(findNamedObject<wxTextCtrl>(this, "MissionInfoEditDialogDescriptionEntry")->GetValue().ToStdString());
		_darkmodTxt->setVersion(findNamedObject<wxTextCtrl>(this, "MissionInfoEditDialogVersionEntry")->GetValue().ToStdString());
		_darkmodTxt->setReqTdmVersion(findNamedObject<wxTextCtrl>(this, "MissionInfoEditDialogReqTdmVersionEntry")->GetValue().ToStdString());

		_guiView->update();
	});
}

void MissionInfoEditDialog::onSave(wxCommandEvent& ev)
{
	try
	{
		// DarkmodTxt is kept in sync all the time, no need to load anything, just save to disk
		_darkmodTxt->saveToCurrentMod();

		// Close the dialog
		EndModal(wxID_OK);
	}
	catch (std::runtime_error& err)
	{
		wxutil::Messagebox::ShowError(err.what(), this);
	}
}

void MissionInfoEditDialog::onCancel(wxCommandEvent& ev)
{
	// destroy dialog without saving
	EndModal(wxID_CANCEL);
}

void MissionInfoEditDialog::onEditReadme(wxCommandEvent& ev)
{
	MissionReadmeDialog* dialog = new MissionReadmeDialog(this);

	dialog->ShowModal();
	dialog->Destroy();
}

void MissionInfoEditDialog::onTitleEdited(wxDataViewEvent& ev)
{
	wxutil::TreeModel::Row row(ev.GetItem(), *_missionTitleStore);

	int titleNum = row[_missionTitleColumns.number].getInteger();

	map::DarkmodTxt::TitleList list = _darkmodTxt->getMissionTitles();

	assert(titleNum >= 0 && titleNum < static_cast<int>(list.size()));

	if (ev.GetColumn() == _missionTitleColumns.title.getColumnIndex())
	{
		list[titleNum] = static_cast<std::string>(ev.GetValue());
		_darkmodTxt->setMissionTitles(list);
	}
}

void MissionInfoEditDialog::onTitleContextMenu(wxDataViewEvent& ev)
{
	_missionTitlesContextMenu->show(findNamedObject<wxWindow>(this, "MissionInfoEditDialogMissionTitleList"));
}

void MissionInfoEditDialog::onAddTitle()
{
	map::DarkmodTxt::TitleList list = _darkmodTxt->getMissionTitles();
	list.push_back("Click to edit Title");
	_darkmodTxt->setMissionTitles(list);

	updateValuesFromDarkmodTxt();
}

void MissionInfoEditDialog::onDeleteTitle()
{
	wxutil::TreeView* treeView = findNamedObject<wxutil::TreeView>(this, "MissionInfoEditDialogMissionTitleList");

	wxDataViewItem item = treeView->GetSelection();

	if (!item.IsOk()) return;

	wxutil::TreeModel::Row row(item, *_missionTitleStore);

	int titleNum = row[_missionTitleColumns.number].getInteger();

	map::DarkmodTxt::TitleList list = _darkmodTxt->getMissionTitles();

	assert(titleNum >= 0 && titleNum < static_cast<int>(list.size()));

	list.erase(list.begin() + titleNum);
	_darkmodTxt->setMissionTitles(list);

	updateValuesFromDarkmodTxt();
}

bool MissionInfoEditDialog::testDeleteTitle()
{
	wxutil::TreeView* treeView = findNamedObject<wxutil::TreeView>(this, "MissionInfoEditDialogMissionTitleList");

	wxDataViewItem item = treeView->GetSelection();

	return item.IsOk();
}

void MissionInfoEditDialog::ShowDialog(const cmd::ArgumentList& args)
{
	MissionInfoEditDialog* instance = new MissionInfoEditDialog;

	instance->ShowModal();
	instance->Destroy();
}

}
