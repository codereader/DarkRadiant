#include "XdFileChooserDialog.h"

#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/ScrolledFrame.h"

#include "i18n.h"
#include "imainframe.h"

#include "ReadableEditorDialog.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/treeview.h>

#include "idialogmanager.h"

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Choose a file...");
}

XdFileChooserDialog::Result XdFileChooserDialog::import(const std::string& defName,
														XData::XDataPtr& newXData,
														std::string& filename,
														XData::XDataLoaderPtr& loader,
														ReadableEditorDialog& editorDialog)
{
	// Import the file:
	XData::XDataMap xdMap;

	if ( loader->importDef(defName,xdMap) )
	{
		if (xdMap.size() > 1)
		{
			// The requested definition has been defined in multiple files. Use the XdFileChooserDialog to pick a file.
			// Optimally, the preview renderer would already show the selected definition.
			XdFileChooserDialog fcDialog(defName, xdMap, editorDialog);
			fcDialog.show();
			if (fcDialog._result == RESULT_CANCEL)
				//User clicked cancel. The window will be destroyed in _postShow()...
				return RESULT_CANCEL;

			XData::XDataMap::iterator ChosenIt = xdMap.find(fcDialog._chosenFile);
			filename = ChosenIt->first;
			newXData = ChosenIt->second;
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

				ui::IDialogPtr dialog = GlobalDialogManager().createMessageBox(_("Problems during import"),
					msg,
					ui::IDialog::MESSAGE_ASK,
					editorDialog.getRefPtr()
					);
				if (dialog->run() == ui::IDialog::RESULT_YES)
				{
					editorDialog.showXdImportSummary();
				}
			}
		}
		return RESULT_OK;
	}

	//Import failed.
	return RESULT_IMPORT_FAILED;
}

XdFileChooserDialog::XdFileChooserDialog(const std::string& defName, const XData::XDataMap& xdMap, ReadableEditorDialog& editorDialog) :
	gtkutil::BlockingTransientWindow(_(WINDOW_TITLE), editorDialog.getRefPtr()),
	_treeview(NULL),
	_result(RESULT_CANCEL),
	_editorDialog(editorDialog),
	_defName(defName)
{
	// Set the default border width in accordance to the HIG
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	// Add a vbox for the dialog elements
	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 6));

	// Create topLabel
	Gtk::Label* topLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(
		_("The requested definition has been found in multiple Files. Choose the file:")));

	// Create the list of files:
	_listStore = Gtk::ListStore::create(_columns);

	_treeview = Gtk::manage(new Gtk::TreeView(_listStore));
	_treeview->append_column(*Gtk::manage(new gtkutil::TextColumn(_("File"), _columns.name, false)));

	// Append all xdMap-entries to the list.
	for (XData::XDataMap::const_iterator it = xdMap.begin(); it != xdMap.end(); ++it)
	{
		Gtk::TreeModel::Row row = *_listStore->append();

		row[_columns.name] = it->first;
	}

	_treeview->set_headers_visible(false);

	// Connect the selection change signal
	Glib::RefPtr<Gtk::TreeSelection> select = _treeview->get_selection();
	select->set_mode(Gtk::SELECTION_SINGLE);
	select->signal_changed().connect(sigc::mem_fun(*this, &XdFileChooserDialog::onSelectionChanged));

	// Create buttons and add them to an hbox:
	Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));

	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(true, 6));
	hbox->pack_start(*okButton, true, false, 0);
	hbox->pack_start(*cancelButton, true, false, 0);

	//Add everything to the vbox and to the window.
	vbox->pack_start(*topLabel, false, false, 0);
	vbox->pack_start(*Gtk::manage(new gtkutil::ScrolledFrame(*_treeview)), true, true, 0);
	vbox->pack_start(*hbox, false, false, 0);

	add(*vbox);

	//Connect the Signals.
	okButton->signal_clicked().connect(sigc::mem_fun(*this, &XdFileChooserDialog::onOk));
	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &XdFileChooserDialog::onCancel));
}

void XdFileChooserDialog::onOk()
{
	_result = RESULT_OK;

	destroy();
}

void XdFileChooserDialog::onCancel()
{
	destroy();
}

void XdFileChooserDialog::onSelectionChanged()
{
	Gtk::TreeModel::iterator iter = _treeview->get_selection()->get_selected();

	if (iter)
	{
		_chosenFile = Glib::ustring((*iter)[_columns.name]);
		_editorDialog.updateGuiView(getRefPtr(), "", _defName, _chosenFile.substr(_chosenFile.find("/")+1));
	}
}

} // namespace ui
