#include "XdFileChooserDialog.h"

#include "wxutil/dialog/MessageBox.h"

#include "i18n.h"
#include "imainframe.h"

#include "ReadableEditorDialog.h"

#include <wx/stattext.h>
#include <wx/sizer.h>

#include "idialogmanager.h"

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Choose a file...");
}

int XdFileChooserDialog::Import(const std::string& defName,
								XData::XDataPtr& newXData,
								std::string& filename,
								XData::XDataLoaderPtr& loader,
								ReadableEditorDialog* editorDialog)
{
	// Import the file:
	XData::XDataMap xdMap;

	if (loader->importDef(defName,xdMap))
	{
		if (xdMap.size() > 1)
		{
			// The requested definition has been defined in multiple files. Use the XdFileChooserDialog to pick a file.
			// Optimally, the preview renderer would already show the selected definition.
			XdFileChooserDialog* fcDialog = new XdFileChooserDialog(defName, xdMap, editorDialog);

			int result = fcDialog->ShowModal();
			
			if (result == wxID_OK)
			{
				XData::XDataMap::iterator ChosenIt = xdMap.find(fcDialog->_chosenFile);
				filename = ChosenIt->first;
				newXData = ChosenIt->second;
			}

			fcDialog->Destroy();

			return result;
		}
		else
		{
			filename = xdMap.begin()->first;
			newXData = xdMap.begin()->second;

			if (loader->getImportSummary().size() > 1)
			{
				std::string msg = (boost::format(_("%s successfully imported.")) % defName).str();
				msg += "\n\nHowever, there were some problems.\n\n";
				msg += _("Do you want to open the import summary?");

				wxutil::Messagebox dialog(_("Problems during import"),
					msg,
					ui::IDialog::MESSAGE_ASK, editorDialog
				);

				if (dialog.run() == ui::IDialog::RESULT_YES)
				{
					editorDialog->showXdImportSummary();
				}
			}
		}

		return wxID_OK;
	}

	throw ImportFailedException(_("Import failed"));
}

XdFileChooserDialog::XdFileChooserDialog(const std::string& defName, 
										 const XData::XDataMap& xdMap, 
										 ReadableEditorDialog* editorDialog) :
	DialogBase(_(WINDOW_TITLE), editorDialog),
	_listStore(new wxutil::TreeModel(_columns, true)),
	_treeview(NULL),
	_editorDialog(editorDialog),
	_defName(defName)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	// Add a vbox for the dialog elements
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	GetSizer()->Add(vbox, 1, wxEXPAND | wxALL, 12);

	// Create topLabel
	wxStaticText* topLabel = new wxStaticText(this, wxID_ANY,
		_("The requested definition has been found in multiple Files. Choose the file:"));

	// Create the list of files:
	_treeview = wxutil::TreeView::CreateWithModel(this, _listStore, wxDV_NO_HEADER);

	_treeview->AppendTextColumn(_("File"), _columns.name.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	// Append all xdMap-entries to the list.
	for (XData::XDataMap::const_iterator it = xdMap.begin(); it != xdMap.end(); ++it)
	{
		wxutil::TreeModel::Row row = _listStore->AddItem();

		row[_columns.name] = it->first;

		row.SendItemAdded();
	}

	// Connect the selection change signal
	_treeview->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxDataViewEventHandler(XdFileChooserDialog::onSelectionChanged), NULL, this);

	//Add everything to the vbox and to the window.
	vbox->Add(topLabel, 0, wxBOTTOM, 6);
	vbox->Add(_treeview, 1, wxEXPAND | wxBOTTOM, 6);
	vbox->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT);
}

void XdFileChooserDialog::onSelectionChanged(wxDataViewEvent& ev)
{
	wxDataViewItem item = _treeview->GetSelection();

	if (item.IsOk())
	{
		wxutil::TreeModel::Row row(item, *_listStore);
		_chosenFile = row[_columns.name];

		_editorDialog->updateGuiView(this, "", _defName, _chosenFile.substr(_chosenFile.find("/")+1));
	}
}

} // namespace ui
