// Project related
#include "ReadableEditorDialog.h"
#include "XdFileChooserDialog.h"
#include "XDataSelector.h"
#include "GuiSelector.h"
#include "gui/GuiManager.h"
#include "TextViewInfoDialog.h"
#include "ReadableGuiView.h"

// General
#include "selectionlib.h"
#include "gtkutil/MultiMonitor.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/FramedWidget.h"
#include "gtkutil/dialog/MessageBox.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/StockIconMenuItem.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/label.h>
#include <gtkmm/paned.h>
#include <gtkmm/entry.h>
#include <gtkmm/image.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/table.h>
#include <gtkmm/textview.h>
#include <gtkmm/menu.h>
#include <gtkmm/menuitem.h>

#include <gdk/gdkkeysyms.h>

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem.hpp>

#include "registry/registry.h"
#include "string/string.h"
#include "i18n.h"

// Modules
#include "imainframe.h"
#include "ientity.h"
#include "imap.h"
#include "igame.h"
#include "idialogmanager.h"

#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/spinctrl.h>
#include <wx/radiobut.h>

namespace ui
{

// consts:
namespace
{
	const char* const WINDOW_TITLE = N_("Readable Editor");

	const char* const NO_ENTITY_ERROR = N_("Cannot run Readable Editor on this selection.\n"
		"Please select a single XData entity.");

	const char* const LABEL_PAGE_RELATED = N_("Page Editing:");
	const char* const LABEL_GENERAL_PROPERTIES = N_("General Properties:");

	const int MIN_ENTRY_WIDTH = 35;

} // namespace

//////////////////////////////////////////////////////////////////////////////
// Public and protected methods:
ReadableEditorDialog::ReadableEditorDialog(Entity* entity) :
	DialogBase(_(WINDOW_TITLE)),
	_guiView(Gtk::manage(new gui::ReadableGuiView)),
	_entity(entity),
	_xdLoader(new XData::XDataLoader()),
	_currentPageIndex(0),
	_xdNameSpecified(false),
	_runningGuiLayoutCheck(false),
	_runningXDataUniquenessCheck(false),
	_useDefaultFilename(true),
	_saveInProgress(false)
{
	loadNamedPanel(this, "ReadableEditorMainPanel");

	setupGeneralPropertiesInterface();
	setupPageRelatedInterface();
	setupButtonPanel();

	createMenus();
}

ReadableEditorDialog::~ReadableEditorDialog()
{
	// greebo: Disconnect the focus out event, this can get called even after this dialog has been destroyed
	_xDataNameFocusOut.disconnect();
}

int ReadableEditorDialog::ShowModal()
{
	// Load the initial values from the entity
	if (!initControlsFromEntity())
	{
		// User clicked cancel, so destroy the window.
		return wxID_CANCEL;
	}

	// Initialize proper editing controls.
	populateControlsFromXData();

	// Initialise the GL widget after the widgets have been shown
	_guiView->initialiseView();

	return DialogBase::ShowModal();
}

void ReadableEditorDialog::RunDialog(const cmd::ArgumentList& args)
{
	// Check prerequisites
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	if (info.entityCount == 1 && info.totalCount == info.entityCount)
	{
		// Check the entity type
		Entity* entity = Node_getEntity(GlobalSelectionSystem().ultimateSelected());

		if (entity != NULL && entity->getKeyValue("editor_readable") == "1")
		{
			// Show the dialog
			ReadableEditorDialog* dialog = new ReadableEditorDialog(entity);

			dialog->ShowModal();
			dialog->Destroy();
			return;
		}
	}

	// Exactly one redable entity must be selected.
	wxutil::Messagebox::ShowError(_(NO_ENTITY_ERROR));
}

// UI Creation

void ReadableEditorDialog::setupGeneralPropertiesInterface()
{
	makeLabelBold(this, "ReadableEditorGeneralLabel");
	makeLabelBold(this, "ReadableEditorPageLabel");

	// Readable Name
	_nameEntry = findNamedObject<wxTextCtrl>(this, "ReadableEditorInventoryName");
	_nameEntry->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(ReadableEditorDialog::onKeyPress), NULL, this);

	// XData Name
	_xDataNameEntry = findNamedObject<wxTextCtrl>(this, "ReadableEditorXDataName");
	_xDataNameEntry->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(ReadableEditorDialog::onKeyPress), NULL, this);
	_xDataNameEntry->Connect(wxEVT_KILL_FOCUS, wxFocusEventHandler(ReadableEditorDialog::onFocusOut), NULL, this);

	// Add a browse-button.
	findNamedObject<wxButton>(this, "ReadableEditorXDBrowseButton")->Connect(wxEVT_BUTTON,
		wxCommandEventHandler(ReadableEditorDialog::onBrowseXd), NULL, this);

	// Page count
	_numPages = findNamedObject<wxSpinCtrl>(this, "ReadableEditorNumPages");
	_numPages->SetRange(1, 20);
	_numPages->Connect(wxEVT_SPINCTRL, wxSpinEventHandler(ReadableEditorDialog::onNumPagesChanged), NULL, this);
	_numPages->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(ReadableEditorDialog::onKeyPress), NULL, this);

	// Page Layout:
	_oneSidedButton = findNamedObject<wxRadioButton>(this, "ReadableEditorOneSided");
	_oneSidedButton->Connect(wxEVT_RADIOBUTTON, wxCommandEventHandler(ReadableEditorDialog::onOneSided), NULL, this);

    _twoSidedButton = findNamedObject<wxRadioButton>(this, "ReadableEditorTwoSided");
	_twoSidedButton->Connect(wxEVT_RADIOBUTTON, wxCommandEventHandler(ReadableEditorDialog::onTwoSided), NULL, this);

	// Pageturn Sound
	_pageTurnEntry = findNamedObject<wxTextCtrl>(this, "ReadableEditorPageTurnSound");
}

void ReadableEditorDialog::setupPageRelatedInterface()
{
	// Insert Button
	findNamedObject<wxButton>(this, "ReadableEditorInsertPage")->Connect(wxEVT_BUTTON,
		wxCommandEventHandler(ReadableEditorDialog::onInsert), NULL, this);

	// Delete Button
	findNamedObject<wxButton>(this, "ReadableEditorDeletePage")->Connect(wxEVT_BUTTON,
		wxCommandEventHandler(ReadableEditorDialog::onDelete), NULL, this);
	
	// Page Switcher
	findNamedObject<wxButton>(this, "ReadableEditorGotoFirstPage")->Connect(wxEVT_BUTTON,
		wxCommandEventHandler(ReadableEditorDialog::onFirstPage), NULL, this);
	findNamedObject<wxButton>(this, "ReadableEditorGotoPreviousPage")->Connect(wxEVT_BUTTON,
		wxCommandEventHandler(ReadableEditorDialog::onPrevPage), NULL, this);
	findNamedObject<wxButton>(this, "ReadableEditorGotoNextPage")->Connect(wxEVT_BUTTON,
		wxCommandEventHandler(ReadableEditorDialog::onNextPage), NULL, this);
	findNamedObject<wxButton>(this, "ReadableEditorGotoLastPage")->Connect(wxEVT_BUTTON,
		wxCommandEventHandler(ReadableEditorDialog::onLastPage), NULL, this);

	_curPageDisplay = findNamedObject<wxStaticText>(this, "ReadableEditorCurPage");

	// Add a gui chooser with a browse-button
	_guiEntry = findNamedObject<wxTextCtrl>(this, "ReadableEditorGuiDefinition");
	_guiEntry->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(ReadableEditorDialog::onKeyPress), NULL, this);
	_guiEntry->Connect(wxEVT_KILL_FOCUS, wxFocusEventHandler(ReadableEditorDialog::onFocusOut), NULL, this);

	findNamedObject<wxButton>(this, "ReadableEditorGuiBrowseButton")->Connect(wxEVT_BUTTON,
		wxCommandEventHandler(ReadableEditorDialog::onBrowseGui), NULL, this);

	// Create "left" and "right" labels and add them to the first row of the table
	_pageLeftLabel = findNamedObject<wxStaticText>(this, "ReadableEditorPageLeftLabel");
	_pageRightLabel = findNamedObject<wxStaticText>(this, "ReadableEditorPageRightLabel");

	_textViewTitle = findNamedObject<wxTextCtrl>(this, "ReadableEditorTitleLeft");
	_textViewTitle->Connect(wxEVT_TEXT, wxCommandEventHandler(ReadableEditorDialog::onTextChanged), NULL, this);

	_textViewRightTitle = findNamedObject<wxTextCtrl>(this, "ReadableEditorTitleRight");
	_textViewRightTitle->Connect(wxEVT_TEXT, wxCommandEventHandler(ReadableEditorDialog::onTextChanged), NULL, this);

	// Create "body" label and body-textViews and add them to the third row of the table. Add the key-press-event.
	_textViewBody = findNamedObject<wxTextCtrl>(this, "ReadableEditorBodyLeft");
	_textViewBody->Connect(wxEVT_TEXT, wxCommandEventHandler(ReadableEditorDialog::onTextChanged), NULL, this);

	_textViewRightBody = findNamedObject<wxTextCtrl>(this, "ReadableEditorBodyRight");
	_textViewRightBody->Connect(wxEVT_TEXT, wxCommandEventHandler(ReadableEditorDialog::onTextChanged), NULL, this);
}

void ReadableEditorDialog::createMenus()
{
	// Insert menu
	_insertMenu = Gtk::manage(new Gtk::Menu);

	Gtk::MenuItem* insWhole = Gtk::manage(new Gtk::MenuItem(_("Insert whole Page")));
	insWhole->signal_activate().connect(sigc::mem_fun(*this, &ReadableEditorDialog::onInsertWhole));

	_insertMenu->append(*insWhole);

	Gtk::MenuItem* insLeft = Gtk::manage(new Gtk::MenuItem(_("Insert on left Side")));
	insLeft->signal_activate().connect(sigc::mem_fun(*this, &ReadableEditorDialog::onInsertLeft));

	_insertMenu->append(*insLeft);

	Gtk::MenuItem* insRight = Gtk::manage(new Gtk::MenuItem(_("Insert on right Side")));
	insRight->signal_activate().connect(sigc::mem_fun(*this, &ReadableEditorDialog::onInsertRight));

	_insertMenu->append(*insRight);

	_insertMenu->show_all();

	// Delete Menu
	_deleteMenu = Gtk::manage(new Gtk::Menu);

	Gtk::MenuItem* delWhole = Gtk::manage(new Gtk::MenuItem(_("Delete whole Page")));
	delWhole->signal_activate().connect(sigc::mem_fun(*this, &ReadableEditorDialog::onDeleteWhole));

	_deleteMenu->append(*delWhole);

	Gtk::MenuItem* delLeft = Gtk::manage(new Gtk::MenuItem(_("Delete on left Side")));
	delLeft->signal_activate().connect(sigc::mem_fun(*this, &ReadableEditorDialog::onDeleteLeft));

	_deleteMenu->append(*delLeft);

	Gtk::MenuItem* delRight = Gtk::manage(new Gtk::MenuItem(_("Delete on right Side")));
	delRight->signal_activate().connect(sigc::mem_fun(*this, &ReadableEditorDialog::onDeleteRight));

	_deleteMenu->append(*delRight);

	_deleteMenu->show_all();

	// Append Menu
	_appendMenu = Gtk::manage(new Gtk::Menu);

	Gtk::MenuItem* append = Gtk::manage(new gtkutil::StockIconMenuItem(Gtk::Stock::ADD, _("Append Page")));
	append->signal_activate().connect(sigc::mem_fun(*this, &ReadableEditorDialog::onMenuAppend));

	_appendMenu->append(*append);
	_appendMenu->show_all();

	// Prepend Menu
	_prependMenu = Gtk::manage(new Gtk::Menu);

	Gtk::MenuItem* prepend = Gtk::manage(new gtkutil::StockIconMenuItem(Gtk::Stock::ADD, _("Prepend Page")));
	prepend->signal_activate().connect(sigc::mem_fun(*this, &ReadableEditorDialog::onMenuPrepend));

	_prependMenu->append(*prepend);
	_prependMenu->show_all();

	// Tools Menu
	_toolsMenu = Gtk::manage(new Gtk::Menu);

	Gtk::MenuItem* impSum = Gtk::manage(new gtkutil::StockIconMenuItem(Gtk::Stock::DND, _("Show last XData import summary")));
	impSum->signal_activate().connect(sigc::mem_fun(*this, &ReadableEditorDialog::onXdImpSum));

	_toolsMenu->append(*impSum);

	Gtk::MenuItem* dupDef = Gtk::manage(new gtkutil::StockIconMenuItem(Gtk::Stock::COPY, _("Show duplicated definitions")));
	dupDef->signal_activate().connect(sigc::mem_fun(*this, &ReadableEditorDialog::onDupDef));

	_toolsMenu->append(*dupDef);

	Gtk::MenuItem* guiImp = Gtk::manage(new gtkutil::StockIconMenuItem(Gtk::Stock::DND, _("Show Gui import summary")));
	guiImp->signal_activate().connect(sigc::mem_fun(*this, &ReadableEditorDialog::onGuiImpSum));

	_toolsMenu->append(*guiImp);

	_toolsMenu->show_all();
}

void ReadableEditorDialog::setupButtonPanel()
{
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(true, 6));

	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));
	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &ReadableEditorDialog::onCancel));

	_saveButton = Gtk::manage(new Gtk::Button(Gtk::Stock::SAVE));
	_saveButton->signal_clicked().connect(sigc::mem_fun(*this, &ReadableEditorDialog::onSave));

	Gtk::Button* saveCloseButton = Gtk::manage(new Gtk::Button(_("Save & Close")));
	saveCloseButton->set_image(*Gtk::manage(new Gtk::Image(Gtk::Stock::APPLY, Gtk::ICON_SIZE_LARGE_TOOLBAR)));
	saveCloseButton->signal_clicked().connect(sigc::mem_fun(*this, &ReadableEditorDialog::onSaveClose));

	Gtk::Button* toolsButton = Gtk::manage(new Gtk::Button(_("Tools")));
	toolsButton->set_image(*Gtk::manage(new Gtk::Image(Gtk::Stock::PREFERENCES, Gtk::ICON_SIZE_LARGE_TOOLBAR)));
	toolsButton->signal_clicked().connect(sigc::mem_fun(*this, &ReadableEditorDialog::onToolsClicked));

	hbx->pack_end(*saveCloseButton, true, true, 0);
	hbx->pack_end(*_saveButton, true, true, 0);
	hbx->pack_end(*cancelButton, true, true, 0);
	hbx->pack_end(*toolsButton, true, true, 18);

	return *Gtk::manage(new gtkutil::RightAlignment(*hbx));
}

//////////////////////////////////////////////////////////////////////////////
// Private Methods:

bool ReadableEditorDialog::initControlsFromEntity()
{
	// Inv_name
	_nameEntry->set_text(_entity->getKeyValue("inv_name"));

	// Xdata contents
	_xDataNameEntry->set_text(_entity->getKeyValue("xdata_contents"));

	// Construct the map-based Filename
	_mapBasedFilename = GlobalMapModule().getMapName();
	std::size_t nameStartPos = _mapBasedFilename.rfind("/") + 1;

	if (nameStartPos != std::string::npos)
	{
		_mapBasedFilename = _mapBasedFilename.substr(nameStartPos, _mapBasedFilename.rfind(".") - nameStartPos);
	}

	std::string defaultXdName = "readables/" + _mapBasedFilename + "/" + _("<Name_Here>");
	_mapBasedFilename += ".xd";

	// Load xdata
	if (!_entity->getKeyValue("xdata_contents").empty())
	{
		_xdNameSpecified = true;

		try
		{
			int result = XdFileChooserDialog::Import(
				_entity->getKeyValue("xdata_contents"), _xData, _xdFilename, _xdLoader, this
			);

			if (result == wxID_OK)
			{
				_useDefaultFilename = false;
				refreshWindowTitle();
				return true;
			}
			
			return false; // cancelled
		}
		catch (XdFileChooserDialog::ImportFailedException&)
		{
			std::string msg = (boost::format(_("Failed to import %s.")) % _entity->getKeyValue("xdata_contents")).str();
			msg += "\n";
			msg += _("Creating a new XData definition...");
			msg += "\n\n";
			msg += _("Do you want to open the import summary?");

			wxutil::Messagebox dialog(_("Import failed"),
				msg, ui::IDialog::MESSAGE_ASK/*, wxTODO getRefPtr()*/);

			if (dialog.run() == ui::IDialog::RESULT_YES)
			{
				showXdImportSummary();
			}

			updateGuiView();
		}
	}

	//No Xdata definition was defined or failed to import. Use default filename and create a OneSidedXData-object
	if (_entity->getKeyValue("name").find("book") == std::string::npos)
	{
		if (!_xdNameSpecified)
			_xData.reset(new XData::OneSidedXData(defaultXdName));
		else
			_xData.reset(new XData::OneSidedXData(_entity->getKeyValue("xdata_contents")));
	}
	else
	{
		if (!_xdNameSpecified)
			_xData.reset(new XData::TwoSidedXData(defaultXdName));
		else
			_xData.reset(new XData::OneSidedXData(_entity->getKeyValue("xdata_contents")));
	}
	_xData->setNumPages(1);

	refreshWindowTitle();

	return true;
}

bool ReadableEditorDialog::save()
{
	_saveInProgress = true;

	// Name
	_entity->setKeyValue("inv_name", _nameEntry->get_text());

	// Xdata contents
	_entity->setKeyValue("xdata_contents", _xDataNameEntry->get_text());

	// Current content to XData Object
	storeXData();

	// Get the storage path and check its validity
	std::string storagePath = constructStoragePath();

	if (!_useDefaultFilename && !boost::filesystem::exists(storagePath))
	{
		// The file does not exist, so we have imported a definition contained inside a PK4.
		wxutil::Messagebox::ShowError(
			_("You have imported an XData definition that is contained in a PK4, which can't be accessed for saving.") +
			std::string("\n\n") +
			_("Please rename your XData definition, so that it is stored under a different filename."),
			NULL/* wxTODO getRefPtr()*/
		);

		_saveInProgress = false;
		return false;
	}

	// Start exporting
	XData::FileStatus fst = _xData->xport(storagePath, XData::Merge);

	if (fst == XData::DefinitionExists)
	{
		switch (_xData->xport( storagePath, XData::MergeOverwriteExisting))
		{
		case XData::OpenFailed:
			wxutil::Messagebox::ShowError(
				(boost::format(_("Failed to open %s for saving.")) % _xdFilename).str(),
				NULL/* wxTODO getRefPtr()*/
			);
			_saveInProgress = false;
			return false;
		case XData::MergeFailed:
			wxutil::Messagebox::ShowError(
				_("Merging failed, because the length of the definition to be overwritten could not be retrieved."),
				NULL/* wxTODO getRefPtr()*/
			);
			_saveInProgress = false;
			return false;
		default:
			//success!
			_saveInProgress = false;
			return true;
		}
	}
	else if (fst == XData::OpenFailed)
	{
		wxutil::Messagebox::ShowError(
			(boost::format(_("Failed to open %s for saving.")) % _xdFilename).str(),
			NULL/* wxTODO getRefPtr()*/
		);
	}

	_saveInProgress = false;
	return false;
}

std::string ReadableEditorDialog::constructStoragePath()
{
	// Construct the storage path from Registry keys.
	std::string storagePath;
	if (_useDefaultFilename)
	{
		bool checkVFS = true;
		switch (registry::getValue<int>(RKEY_READABLES_STORAGE_FOLDER))
		{
			case 0: // Use Mod dir
				storagePath = GlobalGameManager().getModPath();
				if (storagePath.empty())
				{
					// Mod path not defined. Use base Path
					storagePath = GlobalRegistry().get(RKEY_ENGINE_PATH) + "base/";
					wxutil::Messagebox::ShowError(_("Mod path not defined. Using Base path..."), NULL/* wxTODO getRefPtr()*/);
				}
				storagePath += XData::XDATA_DIR;
				break;
			case 1: // Use Mod Base dir
				storagePath = GlobalGameManager().getModBasePath();
				if (storagePath.empty())
				{
					// Mod Base Path not defined. Use Mod path or base path successively.
					storagePath = GlobalGameManager().getModPath();
					if (storagePath.empty())
					{
						storagePath = GlobalRegistry().get(RKEY_ENGINE_PATH) + "base/";
						wxutil::Messagebox::ShowError(_("Mod Base path not defined, neither is Mod path. Using Engine path..."),
							NULL/* wxTODO getRefPtr()*/);
						storagePath += XData::XDATA_DIR;
						break;
					}
					wxutil::Messagebox::ShowError(_("Mod Base path not defined. Using Mod path..."), NULL/* wxTODO getRefPtr()*/);
				}
				storagePath += XData::XDATA_DIR;
				break;
			default: // Use custom folder
				storagePath = GlobalRegistry().get(RKEY_READABLES_CUSTOM_FOLDER);
				if (storagePath.empty())
				{
					// Custom path not defined. Use Mod path or base path successively.
					storagePath = GlobalGameManager().getModPath();
					if (storagePath.empty())
					{
						storagePath = GlobalRegistry().get(RKEY_ENGINE_PATH) + "base/";
						wxutil::Messagebox::ShowError(_("Mod Base path not defined, neither is Mod path. Using Engine path..."), NULL/* wxTODO getRefPtr()*/);
						storagePath += XData::XDATA_DIR;
						break;
					}
					storagePath += XData::XDATA_DIR;
					wxutil::Messagebox::ShowError(_("Mod Base path not defined. Using Mod path..."), NULL/* wxTODO getRefPtr()*/);
					break;
				}
				storagePath += "/" +_mapBasedFilename;
				checkVFS = false;
				break;
		}

		// Check whether this path already exists in the VFS in another absolute folder. If so, rename it!
		if (checkVFS) {
			std::string compPath = GlobalFileSystem().findFile(XData::XDATA_DIR + _mapBasedFilename);
			std::string newFileName = _mapBasedFilename;
			if (!compPath.empty()) { // VFS-path does exist, now check whether it is in another absolute folder.
				int fileIdx = 2;
				compPath += XData::XDATA_DIR + _mapBasedFilename;
				while ( (compPath.compare(storagePath + newFileName) != 0) ) {
					newFileName = _mapBasedFilename.substr(0, _mapBasedFilename.rfind(".")) + boost::lexical_cast<std::string>(fileIdx++) + ".xd";
					compPath = GlobalFileSystem().findFile(XData::XDATA_DIR + newFileName);
					if (compPath.empty())
					{
						wxutil::Messagebox::ShowError(
							(boost::format(_("%s%s already exists in another path.\n\nXData will be stored in %s%s!")) % XData::XDATA_DIR % _mapBasedFilename % XData::XDATA_DIR % newFileName).str(),
							NULL/* wxTODO getRefPtr()*/
						);
						break;
					}
					compPath += XData::XDATA_DIR + newFileName;
				}
			}
			storagePath += newFileName;
		}
	}
	else
	{
		// We are exporting a previously imported XData definition. Retrieve engine path and append _xdFilename
		storagePath = GlobalRegistry().get(RKEY_ENGINE_PATH) + _xdFilename;
	}

	return storagePath;
}

void ReadableEditorDialog::refreshWindowTitle()
{
	std::string title = constructStoragePath();
	title = title.substr( title.find_first_not_of( GlobalRegistry().get(RKEY_ENGINE_PATH) ) );
	title = std::string(_("Readable Editor")) +  "  -  " + title;

	set_title(title);
}

void ReadableEditorDialog::storeXData()
{
	//NumPages does not need storing, because it's stored directly after changing it.
	_xData->setName(_xDataNameEntry->get_text());
	_xData->setSndPageTurn(_pageTurnEntry->get_text());

	storeCurrentPage();
}

void ReadableEditorDialog::toggleTwoSidedEditingInterface(bool show)
{
	if (show)
	{
		_textViewTable->set_col_spacing(1, 12);
		_textViewRightBodyScrolled->show();
		_textViewRightTitleScrolled->show();
		_pageLeftLabel->show();
		_pageRightLabel->show();
	}
	else
	{
		_textViewTable->set_col_spacing(1, 0);
		_textViewRightBodyScrolled->hide();
		_textViewRightTitleScrolled->hide();
		_pageLeftLabel->hide();
		_pageRightLabel->hide();
	}
}

void ReadableEditorDialog::showPage(std::size_t pageIndex)
{
	//Gui Def before:
	std::string guiBefore = _guiEntry->get_text();

	// Update CurrentPage Label
	_currentPageIndex = pageIndex;
	_curPageDisplay->set_text(string::to_string(pageIndex+1));

	if (_xData->getPageLayout() == XData::TwoSided)
	{
		// Update Gui statement entry from xData
		if (!_xData->getGuiPage(pageIndex).empty())
		{
			_guiEntry->set_text(_xData->getGuiPage(pageIndex));
		}
		else
		{
			_guiEntry->set_text(XData::DEFAULT_TWOSIDED_GUI);
		}

		setTextViewAndScroll(_textViewRightTitle, _xData->getPageContent(XData::Title, pageIndex, XData::Right));
		setTextViewAndScroll(_textViewRightBody, _xData->getPageContent(XData::Body, pageIndex, XData::Right));
	}
	else
	{
		// Update Gui statement entry from xData
		if (!_xData->getGuiPage(pageIndex).empty())
		{
			_guiEntry->set_text(_xData->getGuiPage(pageIndex));
		}
		else
		{
			_guiEntry->set_text(XData::DEFAULT_ONESIDED_GUI);
		}
	}

	// Update page statements textviews from xData
	setTextViewAndScroll(_textViewTitle, _xData->getPageContent(XData::Title, pageIndex, XData::Left));
	setTextViewAndScroll(_textViewBody, _xData->getPageContent(XData::Body, pageIndex, XData::Left));

	// Update the GUI View if the gui changed. For the page contents, updateGuiView is called automatically
	// by onTextChange.
	if (guiBefore != _guiEntry->get_text())
	{
		updateGuiView();
	}
}

void ReadableEditorDialog::updateGuiView(const Glib::RefPtr<Gtk::Window>& parent,
										 const std::string& guiPath,
										 const std::string& xDataName,
										 const std::string& xDataPath)
{
	// If the name of an xData object is passed it will be rendered instead of the current
	// xData object, to enable previewing of XData definitions induced by the XDataSelector.
	if (!xDataName.empty())
	{
		XData::XDataPtr xd;
		XData::XDataMap xdMap;

		if (_xdLoader->importDef(xDataName, xdMap, xDataPath))
		{
			xd = xdMap.begin()->second;
			_guiView->setGui(xd->getGuiPage(0));
		}
		else
		{
			std::string msg = (boost::format(_("Failed to import %s.")) % xDataName).str();
			msg += "\n\n";
			msg += _("Do you want to open the import summary?");

			wxutil::Messagebox dialog(_("Import failed"),
				msg,
				ui::IDialog::MESSAGE_ASK/* wxTODO,
				parent ? parent : getRefPtr()*/
			);
			if (dialog.run() == ui::IDialog::RESULT_YES)
			{
				showXdImportSummary();
			}
			return;
		}

		const gui::GuiPtr& gui = _guiView->getGui();

		if (gui == NULL)
		{
			std::string msg = (boost::format(_("Failed to load gui definition %s.")) % xd->getGuiPage(0)).str();
			msg += "\n\n";
			msg += _("Do you want to open the import summary?");

			wxutil::Messagebox dialog(_("Import failed"),
				msg,
				ui::IDialog::MESSAGE_ASK/* wxTODO,
				parent ? parent : getRefPtr()*/
			);
			if (dialog.run() == ui::IDialog::RESULT_YES)
			{
				showGuiImportSummary();
			}
			return;
		}

		// Load data from xdata into the GUI's state variables
		if (xd->getPageLayout() == XData::OneSided)
		{
			// One-sided has title and body
			gui->setStateString("title", xd->getPageContent(XData::Title, 0, XData::Left));
			gui->setStateString("body", xd->getPageContent(XData::Body, 0, XData::Left));
		}
		else
		{
			// Two-sided has four important state strings
			gui->setStateString("left_title", xd->getPageContent(XData::Title, 0, XData::Left));
			gui->setStateString("left_body", xd->getPageContent(XData::Body, 0, XData::Left));

			gui->setStateString("right_title", xd->getPageContent(XData::Title, 0, XData::Right));
			gui->setStateString("right_body", xd->getPageContent(XData::Body, 0, XData::Right));
		}

		// Initialise the time of this GUI
		gui->initTime(0);

		// Run the first frame
		gui->update(16);
	}
	else
	{
		if (guiPath.empty())
		{
			_guiView->setGui(_guiEntry->get_text());
		}
		else
		{
			_guiView->setGui(guiPath);
		}

		const gui::GuiPtr& gui = _guiView->getGui();

		if (gui == NULL)
		{
			std::string nameGui = guiPath.empty()
                                  ? _guiEntry->get_text().raw()
                                  : guiPath;

			std::string msg = (boost::format(_("Failed to load gui definition %s.")) % nameGui).str();
			msg += "\n\n";
			msg += _("Do you want to open the import summary?");

			wxutil::Messagebox dialog(_("Import failed"),
				msg,
				ui::IDialog::MESSAGE_ASK/* wxTODO,
				parent ? parent : getRefPtr()*/
			);
			if (dialog.run() == ui::IDialog::RESULT_YES)
			{
				showGuiImportSummary();
			}
			return;
		}

		// Load data from xdata into the GUI's state variables
		if (_xData->getPageLayout() == XData::OneSided)
		{
			// One-sided has title and body
			gui->setStateString("title", readTextBuffer(_textViewTitle));
			gui->setStateString("body", readTextBuffer(_textViewBody));
		}
		else
		{
			// Two-sided has four important state strings
			gui->setStateString("left_title", readTextBuffer(_textViewTitle));
			gui->setStateString("left_body", readTextBuffer(_textViewBody));

			gui->setStateString("right_title", readTextBuffer(_textViewRightTitle));
			gui->setStateString("right_body", readTextBuffer(_textViewRightBody));
		}

		// Initialise the time of this GUI
		gui->initTime(0);

		// Run the first frame
		gui->update(16);
	}

	_guiView->redraw();
}

void ReadableEditorDialog::storeCurrentPage()
{
	// Store the GUI-Page
	_xData->setGuiPage(_guiEntry->get_text(), _currentPageIndex);

	// On OneSidedXData the Side-enum is discarded, so it's save to call this method
	_xData->setPageContent(XData::Title, _currentPageIndex, XData::Left, readTextBuffer(_textViewTitle));
	_xData->setPageContent(XData::Body, _currentPageIndex, XData::Left, readTextBuffer(_textViewBody));

	if (_xData->getPageLayout() == XData::TwoSided)
	{
		_xData->setPageContent(XData::Title, _currentPageIndex, XData::Right, readTextBuffer(_textViewRightTitle));
		_xData->setPageContent(XData::Body, _currentPageIndex, XData::Right, readTextBuffer(_textViewRightBody));
	}
}

void ReadableEditorDialog::populateControlsFromXData()
{
	toggleTwoSidedEditingInterface(_xData->getPageLayout() == XData::TwoSided);
	showPage(0);

	_xDataNameEntry->set_text(_xData->getName());
	_numPages->set_value(static_cast<double>(_xData->getNumPages()));

	std::string sndString = _xData->getSndPageTurn();

	_pageTurnEntry->set_text(
		sndString.empty() ? XData::DEFAULT_SNDPAGETURN : sndString
	);

	if (_xData->getPageLayout() == XData::TwoSided)
	{
		_twoSidedButton->set_active(true);
	}
	else
	{
		_oneSidedButton->set_active(true);
	}
}

void ReadableEditorDialog::checkXDataUniqueness()
{
	_runningXDataUniquenessCheck = true;

	std::string xdn = _xDataNameEntry->get_text();

	if (_xData->getName() == xdn)
	{
		_runningXDataUniquenessCheck = false;
		return;
	}

	_xdLoader->retrieveXdInfo();

	XData::StringVectorMap::const_iterator it = _xdLoader->getDefinitionList().find(xdn);

	if (it != _xdLoader->getDefinitionList().end())
	{
		// The definition already exists. Ask the user whether it should be imported. If not make a different name suggestion.
		wxutil::Messagebox dialog(
			_("Import definition?"),
			(boost::format(_("The definition %s already exists. Should it be imported?")) % xdn).str(),
			ui::IDialog::MESSAGE_ASK/* wxTODO, getRefPtr()*/
		);

		std::string message = "";

		if (dialog.run() == ui::IDialog::RESULT_YES)
		{
			try
			{
				int result = XdFileChooserDialog::Import( xdn, _xData, _xdFilename, _xdLoader, this);

				if (result == wxID_OK)
				{
					_xdNameSpecified = true;
					_useDefaultFilename = false;
					populateControlsFromXData();
					_runningXDataUniquenessCheck = false;
					refreshWindowTitle();
					return;
				}
			}
			catch (XdFileChooserDialog::ImportFailedException&)
			{
				message = _("Import failed:");
				message += "\n\t" + _xdLoader->getImportSummary()[_xdLoader->getImportSummary().size() - 1];
				message += "\n\n";
				message += _("Consult the import summary for further information.");
				message += "\n\n";
			}
		}

		// Dialog RESULT_NO, XdFileChooserDialog::RESULT_CANCEL or import failed! Make a different name suggestion!
		std::string suggestion;

		for (int n = 1; n > 0; n++)
		{
			suggestion = xdn + string::to_string(n);

			if (_xdLoader->getDefinitionList().find(suggestion) == _xdLoader->getDefinitionList().end())
			{
				// The suggested XData-name does not exist.
				break;
			}
		}

		// Update entry and XData object. Notify the user about the suggestion.
		_xDataNameEntry->set_text(suggestion);
		_xData->setName(suggestion);

		message += (boost::format(_("To avoid duplicated XData definitions "
			"the current definition has been renamed to %s.")) % suggestion).str();

		wxutil::Messagebox::Show(
			_("XData has been renamed."),
			message, ui::IDialog::MESSAGE_CONFIRM/* wxTODO , getRefPtr()*/
		);
	}
	else
	{
		_xData->setName(xdn);
	}

	_xdNameSpecified = true;
	_useDefaultFilename = true;
	_runningXDataUniquenessCheck = false;
	refreshWindowTitle();
}

void ReadableEditorDialog::insertPage()
{
	storeCurrentPage();
	_xData->setNumPages(_xData->getNumPages()+1);

	_numPages->set_value(static_cast<double>(_xData->getNumPages()));

	for (std::size_t n = _xData->getNumPages() - 1; n > _currentPageIndex; n--)
	{
		_xData->setGuiPage(_xData->getGuiPage(n - 1), n);

		_xData->setPageContent(XData::Title, n, XData::Left,
			_xData->getPageContent(XData::Title, n - 1, XData::Left)
		);

		_xData->setPageContent(XData::Body, n, XData::Left,
			_xData->getPageContent(XData::Body, n - 1, XData::Left)
		);
	}

	// New Page:
	_xData->setPageContent(XData::Title, _currentPageIndex, XData::Left, "");
	_xData->setPageContent(XData::Body, _currentPageIndex, XData::Left, "");
	_xData->setGuiPage(_xData->getGuiPage(_currentPageIndex + 1), _currentPageIndex);

	// Right side:
	if (_xData->getPageLayout() == XData::TwoSided)
	{
		for (std::size_t n = _xData->getNumPages() - 1; n > _currentPageIndex; n--)
		{
			_xData->setGuiPage(_xData->getGuiPage(n - 1), n);

			_xData->setPageContent(XData::Title, n, XData::Right,
				_xData->getPageContent(XData::Title, n - 1, XData::Right)
			);

			_xData->setPageContent(XData::Body, n, XData::Right,
				_xData->getPageContent(XData::Body, n - 1, XData::Right)
			);
		}

		// New Page:
		_xData->setPageContent(XData::Title, _currentPageIndex, XData::Right, "");
		_xData->setPageContent(XData::Body, _currentPageIndex, XData::Right, "");
	}

	showPage(_currentPageIndex);
}

void ReadableEditorDialog::deletePage()
{
	if (_currentPageIndex == _xData->getNumPages() - 1)
	{
		if (_currentPageIndex == 0)
		{
			_xData->setNumPages(0);
			_xData->setNumPages(1);
			showPage(0);
			return;
		}

		_numPages->set_value(static_cast<double>(_currentPageIndex));
	}
	else
	{
		for (std::size_t n = _currentPageIndex; n < _xData->getNumPages()-1; n++)
		{
			_xData->setGuiPage(_xData->getGuiPage(n+1), n);

			_xData->setPageContent(XData::Title, n, XData::Left,
				_xData->getPageContent(XData::Title, n+1, XData::Left)
			);

			_xData->setPageContent(XData::Body, n, XData::Left,
				_xData->getPageContent(XData::Body, n+1, XData::Left)
			);
		}

		// Right Side
		if (_xData->getPageLayout() == XData::TwoSided)
		{
			for (std::size_t n = _currentPageIndex; n < _xData->getNumPages()-1; n++)
			{
				_xData->setGuiPage(_xData->getGuiPage(n+1), n);

				_xData->setPageContent(XData::Title, n, XData::Right,
					_xData->getPageContent(XData::Title, n+1, XData::Right)
				);

				_xData->setPageContent(XData::Body, n, XData::Right,
					_xData->getPageContent(XData::Body, n+1, XData::Right)
				);
			}
		}

		_xData->setNumPages(_xData->getNumPages() - 1);

		_numPages->set_value(static_cast<double>(_xData->getNumPages()));

		showPage(_currentPageIndex);
	}
}


void ReadableEditorDialog::deleteSide(bool rightSide)
{
	storeCurrentPage();

	if (!rightSide)
	{
		_xData->setPageContent(XData::Title, _currentPageIndex, XData::Left,
			_xData->getPageContent(XData::Title, _currentPageIndex, XData::Right)
		);

		_xData->setPageContent(XData::Body, _currentPageIndex, XData::Left,
			_xData->getPageContent(XData::Body, _currentPageIndex, XData::Right)
		);
	}

	if (_currentPageIndex < _xData->getNumPages() - 1)
	{
		_xData->setPageContent(XData::Title, _currentPageIndex, XData::Right,
			_xData->getPageContent(XData::Title, _currentPageIndex + 1, XData::Left)
		);

		_xData->setPageContent(XData::Body, _currentPageIndex, XData::Right,
			_xData->getPageContent(XData::Body, _currentPageIndex + 1, XData::Left)
		);

		for (std::size_t n = _currentPageIndex + 1; n < _xData->getNumPages() - 1; n++)
		{
			_xData->setPageContent(XData::Title, n, XData::Left,
				_xData->getPageContent(XData::Title, n, XData::Right));

			_xData->setPageContent(XData::Title, n, XData::Right,
				_xData->getPageContent(XData::Title, n+1, XData::Left));

			_xData->setPageContent(XData::Body, n, XData::Left,
				_xData->getPageContent(XData::Body, n, XData::Right));

			_xData->setPageContent(XData::Body, n, XData::Right,
				_xData->getPageContent(XData::Body, n+1, XData::Left));
		}

		_xData->setPageContent(XData::Title, _xData->getNumPages() - 1, XData::Left,
			_xData->getPageContent(XData::Title, _xData->getNumPages() - 1, XData::Right));

		_xData->setPageContent(XData::Body, _xData->getNumPages() - 1, XData::Left,
			_xData->getPageContent(XData::Body, _xData->getNumPages() - 1, XData::Right));
	}

	if (_xData->getPageContent(XData::Title, _xData->getNumPages() - 1, XData::Left).empty() &&
		_xData->getPageContent(XData::Body, _xData->getNumPages() - 1, XData::Left).empty())
	{
		// The last page has no content anymore, so delete it.
		_numPages->set_value(static_cast<double>(_xData->getNumPages() - 1));
	}
	else
	{
		_xData->setPageContent(XData::Title, _xData->getNumPages() - 1, XData::Right, "");
		_xData->setPageContent(XData::Body, _xData->getNumPages() - 1, XData::Right, "");
	}

	showPage(_currentPageIndex);
}

void ReadableEditorDialog::insertSide(bool rightSide)
{
	storeCurrentPage();

	if (!_xData->getPageContent(XData::Title, _xData->getNumPages()-1, XData::Right).empty() ||
		!_xData->getPageContent(XData::Body, _xData->getNumPages()-1, XData::Right).empty())
	{
		//Last side has content. Raise numPages before shifting
		_numPages->set_value(static_cast<double>(_xData->getNumPages() + 1));
	}

	for (std::size_t n = _xData->getNumPages() - 1; n>_currentPageIndex; n--)
	{
		_xData->setPageContent(XData::Title, n, XData::Right,
			_xData->getPageContent(XData::Title, n, XData::Left));

		_xData->setPageContent(XData::Title, n, XData::Left,
			_xData->getPageContent(XData::Title, n-1, XData::Right));

		_xData->setPageContent(XData::Body, n, XData::Right,
			_xData->getPageContent(XData::Body, n, XData::Left));

		_xData->setPageContent(XData::Body, n, XData::Left,
			_xData->getPageContent(XData::Body, n-1, XData::Right));
	}

	if (!rightSide)
	{
		_xData->setPageContent(XData::Title, _currentPageIndex, XData::Right,
			_xData->getPageContent(XData::Title, _currentPageIndex, XData::Left));

		_xData->setPageContent(XData::Body, _currentPageIndex, XData::Right,
			_xData->getPageContent(XData::Body, _currentPageIndex, XData::Left));

		_xData->setPageContent(XData::Title, _currentPageIndex, XData::Left, "");
		_xData->setPageContent(XData::Body, _currentPageIndex, XData::Left, "");
	}
	else
	{
		_xData->setPageContent(XData::Title, _currentPageIndex, XData::Right, "");
		_xData->setPageContent(XData::Body, _currentPageIndex, XData::Right, "");
	}

	showPage(_currentPageIndex);
}

void ReadableEditorDialog::useOneSidedEditing()
{
	if (_xData->getPageLayout() != XData::OneSided)
	{
		toggleLayout();
	}
}

void ReadableEditorDialog::useTwoSidedEditing()
{
	if (_xData->getPageLayout() != XData::TwoSided)
	{
		toggleLayout();
	}
}

void ReadableEditorDialog::toggleLayout()
{
	// Convert OneSided XData to TwoSided and vice versa and refresh controls
	storeXData();
	_xData->togglePageLayout(_xData);
	populateControlsFromXData();
}

void ReadableEditorDialog::checkGuiLayout()
{
	_runningGuiLayoutCheck = true;

	std::string guiName = _guiEntry->get_text();

	std::string msg;

	gui::GuiType type = gui::GuiManager::Instance().getGuiType(guiName);

	switch (type)
	{
		case gui::NO_READABLE:
			msg = _("The specified gui definition is not a readable.");
			break;
		case gui::ONE_SIDED_READABLE:
			if (_xData->getPageLayout() != XData::OneSided)
			{
				msg = _("The specified gui definition is not suitable for the currently chosen page-layout.");
			}
			else
			{
				_runningGuiLayoutCheck = false;
				updateGuiView();
				return;
			}
			break;
		case gui::TWO_SIDED_READABLE:
			if (_xData->getPageLayout() != XData::TwoSided)
			{
				msg = _("The specified gui definition is not suitable for the currently chosen page-layout.");
			}
			else
			{
				_runningGuiLayoutCheck = false;
				updateGuiView();
				return;
			}
			break;
		case gui::IMPORT_FAILURE:
			msg = _("Failure during import.");
			break;
		case gui::FILE_NOT_FOUND:
			msg = _("The specified Definition does not exist.");
			break;
		default:
			rWarning() << "Invalid GUI type encountered in switch: " << type << std::endl;
			break;
	}

	wxutil::Messagebox dialog(
		_("Not a suitable Gui Definition!"),
		msg + "\n\n" + std::string(_("Start the Gui Browser?")),
		IDialog::MESSAGE_ASK/*, wxTODO getRefPtr()*/);

	if (dialog.run() == ui::IDialog::RESULT_YES)
	{
		XData::PageLayout layoutBefore = _xData->getPageLayout();
		std::string guiName = GuiSelector::Run(_xData->getPageLayout() == XData::TwoSided, this);

		if (!guiName.empty())
		{
			_guiEntry->set_text(guiName);
			_runningGuiLayoutCheck = false;
			updateGuiView();
			return;
		}
		else
		{
			if (_xData->getPageLayout() != layoutBefore)
			{
				toggleLayout();
			}

			// User clicked cancel. Use default layout:
			if (_xData->getPageLayout() == XData::TwoSided)
			{
				_guiEntry->set_text(XData::DEFAULT_TWOSIDED_GUI);
			}
			else
			{
				_guiEntry->set_text(XData::DEFAULT_ONESIDED_GUI);
			}

			updateGuiView();

			wxutil::Messagebox::Show(_("Switching to default Gui..."),
				_("You didn't choose a Gui. Using the default Gui now."),
				ui::IDialog::MESSAGE_CONFIRM, this);
			_runningGuiLayoutCheck = false;
			return;
		}
	}
	// Entry should hold focus.
	_guiEntry->grab_focus();
	_runningGuiLayoutCheck = false;
}

void ReadableEditorDialog::showXdImportSummary()
{
	XData::StringList summary = _xdLoader->getImportSummary();

	if (summary.empty())
	{
		wxutil::Messagebox::ShowError(_("No import summary available. An XData definition has to be imported first..."),
			this);
		return;
	}

	std::string sum;

	for (std::size_t n = 0; n < summary.size(); n++)
	{
		sum += summary[n];
	}

	TextViewInfoDialog::Show(_("XData import summary"), sum, this);
}

void ReadableEditorDialog::showGuiImportSummary()
{
	XData::StringList errors = gui::GuiManager::Instance().getErrorList();
	if (errors.empty())
	{
		wxutil::Messagebox::ShowError(_("No import summary available. Browse Gui Definitions first."), this);
		return;
	}

	std::string summary;

	for (std::size_t n = 0; n < errors.size(); n++)
	{
		summary += errors[n];
	}

	TextViewInfoDialog::Show(_("Gui import summary"), summary, this);
}

//////////////////////////////////////////////////////////////////////////////
// Callback Methods for Signals:

void ReadableEditorDialog::onInsertWhole()
{
	insertPage();
}

void ReadableEditorDialog::onInsertLeft()
{
	insertSide(false);
}

void ReadableEditorDialog::onInsertRight()
{
	insertSide(true);
}

void ReadableEditorDialog::onDeleteWhole()
{
	deletePage();
}

void ReadableEditorDialog::onDeleteLeft()
{
	deleteSide(false);
}

void ReadableEditorDialog::onDeleteRight()
{
	deleteSide(true);
}

void ReadableEditorDialog::onCancel(wxCommandEvent& ev)
{
	EndModal(wxID_CANCEL);
}

void ReadableEditorDialog::onSave(wxCommandEvent& ev)
{
	if (_xdNameSpecified)
	{
		save();
	}
	else
	{
		wxutil::Messagebox::ShowError(_("Please specify an XData name first!"), this);
	}
}

void ReadableEditorDialog::onSaveClose(wxCommandEvent& ev)
{
	if (!_saveInProgress)
	{
		if (_xdNameSpecified)
		{
			if (save())
			{
				// Done, just destroy the window
				EndModal(wxID_OK);
			}
		}
		else
		{
			wxutil::Messagebox::ShowError(_("Please specify an XData name first!"), this);
		}
	}
}

void ReadableEditorDialog::onBrowseXd(wxCommandEvent& ev)
{
	// FileChooser for xd-files
	_xdLoader->retrieveXdInfo();

	std::string res = XDataSelector::run(_xdLoader->getDefinitionList(), this);

	if (res.empty())
	{
		updateGuiView();
		return;
	}

	// Import the file:
	try
	{
		if (XdFileChooserDialog::Import(res, _xData, _xdFilename, _xdLoader, this) == wxID_OK)
		{
			_xdNameSpecified = true;
			_useDefaultFilename = false;
			populateControlsFromXData();
			refreshWindowTitle();
		}
		else
		{
			updateGuiView();
		}
	}
	catch (XdFileChooserDialog::ImportFailedException&)
	{
		std::string msg = (boost::format(_("Failed to import %s.")) % res).str();
		msg += "\n\n";
		msg += _("Do you want to open the import summary?");

		wxutil::Messagebox dialog(_("Import failed"),
			msg,
			ui::IDialog::MESSAGE_ASK, this);

		if (dialog.run() == ui::IDialog::RESULT_YES)
		{
			showXdImportSummary();
		}
		updateGuiView();
		return;
	}
}


void ReadableEditorDialog::onNextPage(wxCommandEvent& ev)
{
	if (_currentPageIndex+1 < _xData->getNumPages())
	{
		storeCurrentPage();
		showPage(_currentPageIndex+1);
	}
	else
	{
		_appendMenu->popup(1, gtk_get_current_event_time());
	}
}

void ReadableEditorDialog::onPrevPage(wxCommandEvent& ev)
{
	if (_currentPageIndex > 0)
	{
		storeCurrentPage();
		showPage(_currentPageIndex-1);
	}
	else
	{
		_prependMenu->popup(1, gtk_get_current_event_time());
	}
}

void ReadableEditorDialog::onFirstPage(wxCommandEvent& ev)
{
	if (_currentPageIndex != 0)
	{
		storeCurrentPage();
		showPage(0);
	}
	else
	{
		_prependMenu->popup(1, gtk_get_current_event_time());
	}
}

void ReadableEditorDialog::onLastPage(wxCommandEvent& ev)
{
	if (_currentPageIndex != _xData->getNumPages() - 1)
	{
		storeCurrentPage();
		showPage(_xData->getNumPages()-1);
	}
	else
	{
		_appendMenu->popup(1, gtk_get_current_event_time());
	}
}

void ReadableEditorDialog::onBrowseGui(wxCommandEvent& ev)
{
	XData::PageLayout layoutBefore = _xData->getPageLayout();
	std::string guiDefBefore = _guiEntry->get_text();
	std::string guiName = GuiSelector::Run(_xData->getPageLayout() == XData::TwoSided, this);

	if (!guiName.empty())
	{
		_guiEntry->set_text(guiName);
	}
	else
	{
		if (_xData->getPageLayout() != layoutBefore)
		{
			toggleLayout();
		}

		if (_guiEntry->get_text() != guiDefBefore)
		{
			_guiEntry->set_text(guiDefBefore);
		}

		updateGuiView();
	}
}

void ReadableEditorDialog::onInsert(wxCommandEvent& ev)
{
	if (_xData->getPageLayout() == XData::TwoSided)
	{
		_insertMenu->popup(1, gtk_get_current_event_time());
	}
	else
	{
		insertPage();
	}
}

void ReadableEditorDialog::onDelete(wxCommandEvent& ev)
{
	if (_xData->getPageLayout() == XData::TwoSided)
	{
		_deleteMenu->popup(1, gtk_get_current_event_time());
	}
	else
	{
		deletePage();
	}
}

void ReadableEditorDialog::onNumPagesChanged(wxSpinEvent& ev)
{
	std::size_t nNP =  static_cast<std::size_t>(_numPages->GetValue());

	_xData->setNumPages(nNP);

	if (_currentPageIndex >= nNP)
	{
		showPage(nNP - 1);
	}
}

void ReadableEditorDialog::onMenuAppend()
{
	_numPages->set_value(static_cast<double>(_xData->getNumPages() + 1));

	storeCurrentPage();
	showPage(_currentPageIndex + 1);
}

void ReadableEditorDialog::onMenuPrepend()
{
	insertPage();
}

void ReadableEditorDialog::onToolsClicked()
{
	_toolsMenu->popup(1, gtk_get_current_event_time());
}

void ReadableEditorDialog::onXdImpSum()
{
	showXdImportSummary();
}

void ReadableEditorDialog::onGuiImpSum()
{
	showGuiImportSummary();
}

void ReadableEditorDialog::onDupDef()
{
	_xdLoader->retrieveXdInfo();

	XData::StringVectorMap dupDefs;

	try
	{
		dupDefs = _xdLoader->getDuplicateDefinitions();
	}
	catch (std::runtime_error&)
	{
		wxutil::Messagebox::Show(
			_("Duplicated XData definitions"),
			_("There are no duplicated definitions!"), 
			ui::IDialog::MESSAGE_CONFIRM, this
		);
		return;
	}

	std::string out;

	for (XData::StringVectorMap::iterator it = dupDefs.begin(); it != dupDefs.end(); ++it)
	{
		std::string occ;

		for (std::size_t n = 0; n < it->second.size()-1; n++)
		{
			occ += it->second[n] + ", ";
		}

		occ += it->second[it->second.size() - 1];

		out += (boost::format(_("%s has been defined in:")) % it->first).str();
		out +="\n\t";
		out += occ;
		out += ".\n\n";
	}

	TextViewInfoDialog::Show(_("Duplicated XData definitions"), out, this);
}

void ReadableEditorDialog::onOneSided()
{
	if ((_oneSidedButton->get_active()))
		useOneSidedEditing();
}

void ReadableEditorDialog::onTwoSided()
{
	if ((_twoSidedButton->get_active()))
		useTwoSidedEditing();
}

//////////////////////////////////////////////////////////////////////////////
// Callback Methods for Events:


void ReadableEditorDialog::onFocusOut(wxFocusEvent& ev)
{
	if (ev.GetEventObject() == _xDataNameEntry)
	{
		// Only call checkXDataUniqueness if the method is not running yet.
		if (!_runningXDataUniquenessCheck)
		{
			checkXDataUniqueness();
		}
	}
	else // WIDGET_GUI_ENTRY
	{
		// Only call checkGuiLayout() if the method is not yet running
		if (!_runningGuiLayoutCheck)
		{
			checkGuiLayout();
		}
	}

	ev.Skip();
}

void ReadableEditorDialog::onTextChanged()
{
	// Update the preview
	updateGuiView();
}

void ReadableEditorDialog::onKeyPress(wxKeyEvent& ev)
{
	bool xdWidget = (widget == _xDataNameEntry);

	if (xdWidget)
	{
		switch (ev->keyval)
		{
			// Some forbidden keys
			case GDK_space:
			case GDK_Tab:
			case GDK_exclam:
			case GDK_asterisk:
			case GDK_plus:
			case GDK_KP_Multiply:
			case GDK_KP_Subtract:
			case GDK_KP_Add:
			case GDK_KP_Separator:
			case GDK_comma:
			case GDK_period:
			case GDK_colon:
			case GDK_semicolon:
			case GDK_question:
			case GDK_minus:
				return true;
			// Check Uniqueness of the XData-Name.
			case GDK_Return:
			case GDK_KP_Enter:
				if (xdWidget)
				{
					checkXDataUniqueness();
				}
				return false;
			default:
				return false;
		}
	}
	else if (widget == _nameEntry)
	{
		switch (ev->keyval)
		{
			// Forbidden key check
			case GDK_Tab:
				return true; // forbid the tab
			default:
				return false;
		}
	}

	if (widget == _numPages)
	{
		if (ev->keyval != GDK_Escape)
		{
			return false;
		}

		// Restore the old value.
		_numPages->set_value(static_cast<double>(_xData->getNumPages()));
		return true;
	}

	if (widget == _guiEntry)
	{
		if (ev->keyval != GDK_Return && ev->keyval != GDK_KP_Enter)
		{
			return false;
		}

		checkGuiLayout();
		return true;
	}

	return false;
}

std::string ReadableEditorDialog::readTextBuffer(Gtk::TextView* view)
{
	Glib::RefPtr<Gtk::TextBuffer> buffer = view->get_buffer();
	return buffer->get_text(true);
}

// Sets the text of a TextView identified by its widget enumerator and scrolls it to the end.
void ReadableEditorDialog::setTextViewAndScroll(Gtk::TextView* view, std::string text)
{
	Glib::RefPtr<Gtk::TextBuffer> buffer = view->get_buffer();

	buffer->set_text(text);

	Gtk::TextBuffer::iterator ending = buffer->end();

	Glib::RefPtr<Gtk::TextMark> markEnd = buffer->create_mark(ending,false);

	view->scroll_to_mark(markEnd, 0); // for some strange reason scroll to iter does not work...
}

} // namespace ui
