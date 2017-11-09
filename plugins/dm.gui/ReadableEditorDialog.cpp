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
#include "wxutil/dialog/MessageBox.h"

#include "registry/registry.h"
#include "string/string.h"
#include "string/convert.h"
#include "os/file.h"
#include "os/path.h"
#include "i18n.h"
#include "itextstream.h"

// Modules
#include "iundo.h"
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
#include <wx/menu.h>

namespace ui
{

// consts:
namespace
{
	const char* const WINDOW_TITLE = N_("Readable Editor");

	const char* const NO_ENTITY_ERROR = N_("Cannot run Readable Editor on this selection.\n"
		"Please select a single XData entity.");

	enum MenuItemId
	{
		InsertWholePage = 1,
		InsertLeft,
		InsertRight,
		DeleteWholePage,
		DeleteLeft,
		DeleteRight,
		AppendPage,
		PrependPage,
		ShowXDataSummary,
		ShowDuplicatedDefs,
		ShowGuiImportSummary,
	};
} // namespace

//////////////////////////////////////////////////////////////////////////////
// Public and protected methods:
ReadableEditorDialog::ReadableEditorDialog(Entity* entity) :
	DialogBase(_(WINDOW_TITLE)),
	_guiView(NULL),
	_entity(entity),
	_xdLoader(new XData::XDataLoader()),
	_currentPageIndex(0),
	_xdNameSpecified(false),
	_runningGuiLayoutCheck(false),
	_runningXDataUniquenessCheck(false),
	_useDefaultFilename(true),
	_saveInProgress(false)
{
	wxPanel* mainPanel = loadNamedPanel(this, "ReadableEditorMainPanel");

	wxPanel* previewPanel = findNamedObject<wxPanel>(this, "ReadableEditorPreviewPanel");
	_guiView = new gui::ReadableGuiView(previewPanel);
	previewPanel->GetSizer()->Add(_guiView, 1, wxEXPAND);

	setupGeneralPropertiesInterface();
	setupPageRelatedInterface();
	setupButtonPanel();
	createMenus();

	mainPanel->Layout();
	mainPanel->Fit();
	Fit();
	CenterOnParent();
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
	_nameEntry->Connect(wxEVT_CHAR, wxKeyEventHandler(ReadableEditorDialog::onChar), NULL, this);

	// XData Name
	_xDataNameEntry = findNamedObject<wxTextCtrl>(this, "ReadableEditorXDataName");
    _xDataNameEntry->Connect(wxEVT_CHAR, wxKeyEventHandler(ReadableEditorDialog::onChar), NULL, this);
	_xDataNameEntry->Connect(wxEVT_KILL_FOCUS, wxFocusEventHandler(ReadableEditorDialog::onFocusOut), NULL, this);

	// Add a browse-button.
	findNamedObject<wxButton>(this, "ReadableEditorXDBrowseButton")->Connect(wxEVT_BUTTON,
		wxCommandEventHandler(ReadableEditorDialog::onBrowseXd), NULL, this);

	// Page count
	_numPages = findNamedObject<wxSpinCtrl>(this, "ReadableEditorNumPages");
	_numPages->SetRange(1, 20);
	_numPages->Connect(wxEVT_SPINCTRL, wxSpinEventHandler(ReadableEditorDialog::onNumPagesChanged), NULL, this);
    _numPages->Connect(wxEVT_CHAR, wxKeyEventHandler(ReadableEditorDialog::onChar), NULL, this);

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
	_guiEntry->Connect(wxEVT_CHAR, wxKeyEventHandler(ReadableEditorDialog::onChar), NULL, this);
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
	_insertMenu.reset(new wxMenu);
	_insertMenu->Append(InsertWholePage, _("Insert whole Page"), "");
	_insertMenu->Append(InsertLeft, _("Insert on left Side"), "");
	_insertMenu->Append(InsertRight, _("Insert on right Side"), "");
	
	_insertMenu->Connect(wxEVT_MENU, wxCommandEventHandler(ReadableEditorDialog::onMenuItemClick), NULL, this);

	// Delete Menu
	_deleteMenu.reset(new wxMenu);
	_deleteMenu->Append(DeleteWholePage, _("Delete whole Page"), "");
	_deleteMenu->Append(DeleteLeft, _("Delete on left Side"), "");
	_deleteMenu->Append(DeleteRight, _("Delete on right Side"), "");

	_deleteMenu->Connect(wxEVT_MENU, wxCommandEventHandler(ReadableEditorDialog::onMenuItemClick), NULL, this);

	// Append Menu
	_appendMenu.reset(new wxMenu);
	_appendMenu->Append(AppendPage, _("Append Page"), "");
	_appendMenu->Connect(wxEVT_MENU, wxCommandEventHandler(ReadableEditorDialog::onMenuItemClick), NULL, this);

	// Prepend Menu
	_prependMenu.reset(new wxMenu);
	_prependMenu->Append(PrependPage, _("Prepend Page"), "");
	_prependMenu->Connect(wxEVT_MENU, wxCommandEventHandler(ReadableEditorDialog::onMenuItemClick), NULL, this);

	// Tools Menu
	_toolsMenu.reset(new wxMenu);
	_toolsMenu->Append(ShowXDataSummary, _("Show last XData import summary"), "");
	_toolsMenu->Append(ShowDuplicatedDefs, _("Show duplicated definitions"), "");
	_toolsMenu->Append(ShowGuiImportSummary, _("Show Gui import summary"), "");
	_toolsMenu->Connect(wxEVT_MENU, wxCommandEventHandler(ReadableEditorDialog::onMenuItemClick), NULL, this);
}

void ReadableEditorDialog::setupButtonPanel()
{
	findNamedObject<wxButton>(this, "ReadableEditorSave")->Connect(wxEVT_BUTTON,
		wxCommandEventHandler(ReadableEditorDialog::onSave), NULL, this);
	findNamedObject<wxButton>(this, "ReadableEditorCancel")->Connect(wxEVT_BUTTON,
		wxCommandEventHandler(ReadableEditorDialog::onCancel), NULL, this);
	findNamedObject<wxButton>(this, "ReadableEditorSaveAndClose")->Connect(wxEVT_BUTTON,
		wxCommandEventHandler(ReadableEditorDialog::onSaveClose), NULL, this);

	findNamedObject<wxButton>(this, "ReadableEditorTools")->Connect(wxEVT_BUTTON,
		wxCommandEventHandler(ReadableEditorDialog::onToolsClicked), NULL, this);
}

//////////////////////////////////////////////////////////////////////////////
// Private Methods:

bool ReadableEditorDialog::initControlsFromEntity()
{
	// Inv_name
	_nameEntry->SetValue(_entity->getKeyValue("inv_name"));

	// Xdata contents
	_xDataNameEntry->SetValue(_entity->getKeyValue("xdata_contents"));

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
			std::string msg = fmt::format(_("Failed to import {0}."), _entity->getKeyValue("xdata_contents"));
			msg += "\n";
			msg += _("Creating a new XData definition...");
			msg += "\n\n";
			msg += _("Do you want to open the import summary?");

			wxutil::Messagebox dialog(_("Import failed"),
				msg, ui::IDialog::MESSAGE_ASK, this);

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

	UndoableCommand cmd("editReadable");

	// Name
	_entity->setKeyValue("inv_name", _nameEntry->GetValue().ToStdString());

	// Xdata contents
	_entity->setKeyValue("xdata_contents", _xDataNameEntry->GetValue().ToStdString());

	// Current content to XData Object
	storeXData();

	// Get the storage path and check its validity
	std::string storagePath = constructStoragePath();

	if (!_useDefaultFilename && !os::fileOrDirExists(storagePath))
	{
		// The file does not exist, so we have imported a definition contained inside a PK4.
		wxutil::Messagebox::ShowError(
			_("You have imported an XData definition that is contained in a PK4, which can't be accessed for saving.") +
			std::string("\n\n") +
			_("Please rename your XData definition, so that it is stored under a different filename."),
			this
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
				fmt::format(_("Failed to open {0} for saving."), _xdFilename),
				this
			);
			_saveInProgress = false;
			return false;
		case XData::MergeFailed:
			wxutil::Messagebox::ShowError(
				_("Merging failed, because the length of the definition to be overwritten could not be retrieved."),
				this
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
			fmt::format(_("Failed to open {0} for saving."), _xdFilename),
			this
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
					wxutil::Messagebox::ShowError(_("Mod path not defined. Using Base path..."), this);
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
							this);
						storagePath += XData::XDATA_DIR;
						break;
					}
					wxutil::Messagebox::ShowError(_("Mod Base path not defined. Using Mod path..."), this);
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
						wxutil::Messagebox::ShowError(_("Mod Base path not defined, neither is Mod path. Using Engine path..."), this);
						storagePath += XData::XDATA_DIR;
						break;
					}
					storagePath += XData::XDATA_DIR;
					wxutil::Messagebox::ShowError(_("Mod Base path not defined. Using Mod path..."), this);
					break;
				}
				storagePath += "/" +_mapBasedFilename;
				checkVFS = false;
				break;
		}

		// Check whether this path already exists in the VFS in another absolute folder. If so, rename it!
		if (checkVFS)
		{
			std::string compPath = GlobalFileSystem().findFile(XData::XDATA_DIR + _mapBasedFilename);
			std::string newFileName = _mapBasedFilename;

			if (!compPath.empty()) // VFS-path does exist, now check whether it is in another absolute folder.
			{ 
				int fileIdx = 2;
				compPath += XData::XDATA_DIR + _mapBasedFilename;

				while ( (compPath.compare(storagePath + newFileName) != 0) )
				{
					newFileName = _mapBasedFilename.substr(0, _mapBasedFilename.rfind(".")) + string::to_string(fileIdx++) + ".xd";
					compPath = GlobalFileSystem().findFile(XData::XDATA_DIR + newFileName);
					if (compPath.empty())
					{
						wxutil::Messagebox::ShowError(
							fmt::format(_("{0}{1} already exists in another path.\n\nXData will be stored in {2}{3}!"),
								XData::XDATA_DIR,
								_mapBasedFilename,
								XData::XDATA_DIR,
								newFileName),
							this
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
		// We are exporting a previously imported XData definition. 
		// The _xdFilename itself can be a VFS path or an absolute path, depending on the user's settings
		if (path_is_absolute(_xdFilename.c_str()))
		{
			storagePath = _xdFilename;
		}
		else
		{
			// Retrieve engine path and append _xdFilename
			storagePath = GlobalRegistry().get(RKEY_ENGINE_PATH) + _xdFilename;
		}
	}

	return storagePath;
}

void ReadableEditorDialog::refreshWindowTitle()
{
	std::string title = constructStoragePath();
	title = title.substr( title.find_first_not_of( GlobalRegistry().get(RKEY_ENGINE_PATH) ) );
	title = std::string(_("Readable Editor")) +  "  -  " + title;

	SetTitle(title);
}

void ReadableEditorDialog::storeXData()
{
	//NumPages does not need storing, because it's stored directly after changing it.
	_xData->setName(_xDataNameEntry->GetValue().ToStdString());
	_xData->setSndPageTurn(_pageTurnEntry->GetValue().ToStdString());

	storeCurrentPage();
}

void ReadableEditorDialog::toggleTwoSidedEditingInterface(bool show)
{
	if (show)
	{
		_textViewRightTitle->GetContainingSizer()->Show(_textViewRightTitle);
		_textViewRightBody->GetContainingSizer()->Show(_textViewRightBody);
		_pageLeftLabel->GetContainingSizer()->Show(_pageLeftLabel);
		_pageRightLabel->GetContainingSizer()->Show(_pageRightLabel);
	}
	else
	{
		_textViewRightTitle->GetContainingSizer()->Hide(_textViewRightTitle);
		_textViewRightBody->GetContainingSizer()->Hide(_textViewRightBody);
		_pageLeftLabel->GetContainingSizer()->Hide(_pageLeftLabel);
		_pageRightLabel->GetContainingSizer()->Hide(_pageRightLabel);
	}

	_textViewRightTitle->GetContainingSizer()->Layout();
}

void ReadableEditorDialog::showPage(std::size_t pageIndex)
{
	//Gui Def before:
	std::string guiBefore = _guiEntry->GetValue().ToStdString();

	// Update CurrentPage Label
	_currentPageIndex = pageIndex;
	_curPageDisplay->SetLabel(string::to_string(pageIndex+1));

	if (_xData->getPageLayout() == XData::TwoSided)
	{
		// Update Gui statement entry from xData
		if (!_xData->getGuiPage(pageIndex).empty())
		{
			_guiEntry->SetValue(_xData->getGuiPage(pageIndex));
		}
		else
		{
			_guiEntry->SetValue(XData::DEFAULT_TWOSIDED_GUI);
		}

		setTextViewAndScroll(_textViewRightTitle, _xData->getPageContent(XData::Title, pageIndex, XData::Right));
		setTextViewAndScroll(_textViewRightBody, _xData->getPageContent(XData::Body, pageIndex, XData::Right));
	}
	else
	{
		// Update Gui statement entry from xData
		if (!_xData->getGuiPage(pageIndex).empty())
		{
			_guiEntry->SetValue(_xData->getGuiPage(pageIndex));
		}
		else
		{
			_guiEntry->SetValue(XData::DEFAULT_ONESIDED_GUI);
		}
	}

	// Update page statements textviews from xData
	setTextViewAndScroll(_textViewTitle, _xData->getPageContent(XData::Title, pageIndex, XData::Left));
	setTextViewAndScroll(_textViewBody, _xData->getPageContent(XData::Body, pageIndex, XData::Left));

	// Update the GUI View if the gui changed. For the page contents, updateGuiView is called automatically
	// by onTextChange.
	if (guiBefore != _guiEntry->GetValue().ToStdString())
	{
		updateGuiView();
	}
}

void ReadableEditorDialog::updateGuiView(wxWindow* parent,
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
			_guiView->setGui(GlobalGuiManager().getGui(xd->getGuiPage(0)));
		}
		else
		{
			std::string msg = fmt::format(_("Failed to import {0}."), xDataName);
			msg += "\n\n";
			msg += _("Do you want to open the import summary?");

			wxutil::Messagebox dialog(_("Import failed"),
				msg, ui::IDialog::MESSAGE_ASK, parent ? parent : this
			);

			if (dialog.run() == ui::IDialog::RESULT_YES)
			{
				showXdImportSummary();
			}
			return;
		}

		const gui::IGuiPtr& gui = _guiView->getGui();

		if (gui == NULL)
		{
			std::string msg = fmt::format(_("Failed to load gui definition {0}."), xd->getGuiPage(0));
			msg += "\n\n";
			msg += _("Do you want to open the import summary?");

			wxutil::Messagebox dialog(_("Import failed"),
				msg, ui::IDialog::MESSAGE_ASK, parent ? parent : this
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

        initGuiState(gui);
	}
	else
	{
		if (guiPath.empty())
		{
			_guiView->setGui(GlobalGuiManager().getGui(_guiEntry->GetValue().ToStdString()));
		}
		else
		{
			_guiView->setGui(GlobalGuiManager().getGui(guiPath));
		}

		const gui::IGuiPtr& gui = _guiView->getGui();

		if (gui == NULL)
		{
			std::string nameGui = guiPath.empty() ? _guiEntry->GetValue().ToStdString() : guiPath;

			std::string msg = fmt::format(_("Failed to load gui definition {0}."), nameGui);
			msg += "\n\n";
			msg += _("Do you want to open the import summary?");

			wxutil::Messagebox dialog(_("Import failed"),
				msg, ui::IDialog::MESSAGE_ASK, parent ? parent : this
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
			gui->setStateString("title", _textViewTitle->GetValue().ToStdString());
			gui->setStateString("body", _textViewBody->GetValue().ToStdString());
		}
		else
		{
			// Two-sided has four important state strings
			gui->setStateString("left_title", _textViewTitle->GetValue().ToStdString());
			gui->setStateString("left_body", _textViewBody->GetValue().ToStdString());

			gui->setStateString("right_title", _textViewRightTitle->GetValue().ToStdString());
			gui->setStateString("right_body", _textViewRightBody->GetValue().ToStdString());
		}

		// Set common GUI state for rendering
        initGuiState(gui);
	}

	_guiView->redraw();
}

void ReadableEditorDialog::initGuiState(const gui::IGuiPtr& gui)
{
    assert(gui);

    // Some fancy readables have custom backgrounds depending on the page number
    // This is usually done by game script at runtime, so let's fake something similar
    gui->setStateString("curPage", string::to_string(_currentPageIndex + 1));
    gui->setStateString("numPages", string::to_string(_numPages->GetValue()));

#if 0
    // ContentsFadeIn is reponsible of setting custom backgrounds
    if (gui->findWindowDef("ContentsFadeIn"))
    {
        gui->findWindowDef("ContentsFadeIn")->notime = false;
    }
#endif

    // Initialise the time of this GUI
	gui->initTime(0);

	// Run the first frame
	gui->update(16);
}

void ReadableEditorDialog::storeCurrentPage()
{
	// Store the GUI-Page
	_xData->setGuiPage(_guiEntry->GetValue().ToStdString(), _currentPageIndex);

	// On OneSidedXData the Side-enum is discarded, so it's save to call this method
	_xData->setPageContent(XData::Title, _currentPageIndex, XData::Left, _textViewTitle->GetValue().ToStdString());
	_xData->setPageContent(XData::Body, _currentPageIndex, XData::Left, _textViewBody->GetValue().ToStdString());

	if (_xData->getPageLayout() == XData::TwoSided)
	{
		_xData->setPageContent(XData::Title, _currentPageIndex, XData::Right, _textViewRightTitle->GetValue().ToStdString());
		_xData->setPageContent(XData::Body, _currentPageIndex, XData::Right, _textViewRightBody->GetValue().ToStdString());
	}
}

void ReadableEditorDialog::populateControlsFromXData()
{
	toggleTwoSidedEditingInterface(_xData->getPageLayout() == XData::TwoSided);
	showPage(0);

	_xDataNameEntry->SetValue(_xData->getName());
	_numPages->SetValue(static_cast<int>(_xData->getNumPages()));

	std::string sndString = _xData->getSndPageTurn();

	_pageTurnEntry->SetValue(
		sndString.empty() ? XData::DEFAULT_SNDPAGETURN : sndString
	);

	if (_xData->getPageLayout() == XData::TwoSided)
	{
		_twoSidedButton->SetValue(true);
	}
	else
	{
		_oneSidedButton->SetValue(true);
	}
}

void ReadableEditorDialog::checkXDataUniqueness()
{
	_runningXDataUniquenessCheck = true;

	std::string xdn = _xDataNameEntry->GetValue().ToStdString();

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
			fmt::format(_("The definition {0} already exists. Should it be imported?"), xdn),
			ui::IDialog::MESSAGE_ASK, this
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
		_xDataNameEntry->SetValue(suggestion);
		_xData->setName(suggestion);

		message += fmt::format(_("To avoid duplicated XData definitions "
			"the current definition has been renamed to {0}."), suggestion);

		wxutil::Messagebox::Show(
			_("XData has been renamed."),
			message, ui::IDialog::MESSAGE_CONFIRM, this
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

	_numPages->SetValue(static_cast<int>(_xData->getNumPages()));
	handleNumberOfPagesChanged();

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

		_numPages->SetValue(static_cast<int>(_currentPageIndex));
		handleNumberOfPagesChanged();
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

		_numPages->SetValue(static_cast<int>(_xData->getNumPages()));

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
		_numPages->SetValue(static_cast<int>(_xData->getNumPages() - 1));
		handleNumberOfPagesChanged();
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
		_numPages->SetValue(static_cast<int>(_xData->getNumPages() + 1));
		handleNumberOfPagesChanged();
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

	std::string guiName = _guiEntry->GetValue().ToStdString();

	std::string msg;

	gui::GuiType type = GlobalGuiManager().getGuiType(guiName);

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
		IDialog::MESSAGE_ASK, this);

	if (dialog.run() == ui::IDialog::RESULT_YES)
	{
		XData::PageLayout layoutBefore = _xData->getPageLayout();
		std::string selectedGuiName = GuiSelector::Run(_xData->getPageLayout() == XData::TwoSided, this);

		if (!selectedGuiName.empty())
		{
			_guiEntry->SetValue(selectedGuiName);
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
				_guiEntry->SetValue(XData::DEFAULT_TWOSIDED_GUI);
			}
			else
			{
				_guiEntry->SetValue(XData::DEFAULT_ONESIDED_GUI);
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
	_guiEntry->SetFocus();
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
	XData::StringList errors = GlobalGuiManager().getErrorList();
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

void ReadableEditorDialog::onMenuItemClick(wxCommandEvent& ev)
{
	switch (static_cast<MenuItemId>(ev.GetId()))
	{
	case InsertWholePage:
		insertPage();
		break;
	case InsertLeft:
		insertSide(false);
		break;
	case InsertRight:
		insertSide(true);
		break;

	case DeleteWholePage:
		deletePage();
		break;
	case DeleteLeft:
		deleteSide(false);
		break;
	case DeleteRight:
		deleteSide(true);
		break;

	case AppendPage:
		_numPages->SetValue(static_cast<int>(_xData->getNumPages() + 1));
		handleNumberOfPagesChanged();
		storeCurrentPage();
		showPage(_currentPageIndex + 1);
		break;

	case PrependPage:
		insertPage();
		break;

	case ShowXDataSummary:
		showXdImportSummary();
		break;
	case ShowGuiImportSummary:
		showGuiImportSummary();
		break;
	case ShowDuplicatedDefs:
		showDuplicateDefinitions();
		break;
	};
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
		std::string msg = fmt::format(_("Failed to import {0}."), res);
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
		static_cast<wxWindow*>(ev.GetEventObject())->PopupMenu(_appendMenu.get());
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
		static_cast<wxWindow*>(ev.GetEventObject())->PopupMenu(_prependMenu.get());
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
		static_cast<wxWindow*>(ev.GetEventObject())->PopupMenu(_prependMenu.get());
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
		static_cast<wxWindow*>(ev.GetEventObject())->PopupMenu(_appendMenu.get());
	}
}

void ReadableEditorDialog::onBrowseGui(wxCommandEvent& ev)
{
	XData::PageLayout layoutBefore = _xData->getPageLayout();
	std::string guiDefBefore = _guiEntry->GetValue().ToStdString();
	std::string guiName = GuiSelector::Run(_xData->getPageLayout() == XData::TwoSided, this);

	if (!guiName.empty())
	{
		_guiEntry->SetValue(guiName);
	}
	else
	{
		if (_xData->getPageLayout() != layoutBefore)
		{
			toggleLayout();
		}

		if (_guiEntry->GetValue() != guiDefBefore)
		{
			_guiEntry->SetValue(guiDefBefore);
		}

		updateGuiView();
	}
}

void ReadableEditorDialog::onInsert(wxCommandEvent& ev)
{
	if (_xData->getPageLayout() == XData::TwoSided)
	{
		static_cast<wxWindow*>(ev.GetEventObject())->PopupMenu(_insertMenu.get());
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
		static_cast<wxWindow*>(ev.GetEventObject())->PopupMenu(_deleteMenu.get());
	}
	else
	{
		deletePage();
	}
}

void ReadableEditorDialog::handleNumberOfPagesChanged()
{
	std::size_t nNP =  static_cast<std::size_t>(_numPages->GetValue());

	_xData->setNumPages(nNP);

	if (_currentPageIndex >= nNP)
	{
		showPage(nNP - 1);
	}
}

void ReadableEditorDialog::onNumPagesChanged(wxSpinEvent& ev)
{
	handleNumberOfPagesChanged();
}

void ReadableEditorDialog::onToolsClicked(wxCommandEvent& ev)
{
	static_cast<wxWindow*>(ev.GetEventObject())->PopupMenu(_toolsMenu.get());
}

void ReadableEditorDialog::showDuplicateDefinitions()
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

		out += fmt::format(_("{0} has been defined in:"), it->first);
		out +="\n\t";
		out += occ;
		out += ".\n\n";
	}

	TextViewInfoDialog::Show(_("Duplicated XData definitions"), out, this);
}

void ReadableEditorDialog::onOneSided(wxCommandEvent& ev)
{
	if ((_oneSidedButton->GetValue()))
		useOneSidedEditing();
}

void ReadableEditorDialog::onTwoSided(wxCommandEvent& ev)
{
	if ((_twoSidedButton->GetValue()))
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

void ReadableEditorDialog::onTextChanged(wxCommandEvent& ev)
{
	// Update the preview
	updateGuiView();
}

void ReadableEditorDialog::onChar(wxKeyEvent& ev)
{
	if (ev.GetEventObject() == _xDataNameEntry)
	{
		switch (ev.GetKeyCode())
		{
		// Some forbidden keys
		case WXK_SPACE:
		case '!':
		case '*':
		case '+':
		case WXK_NUMPAD_MULTIPLY:
		case WXK_NUMPAD_SUBTRACT:
		case WXK_NUMPAD_ADD:
		case WXK_NUMPAD_SEPARATOR:
		case ',':
		case '.':
		case ':':
		case ';':
		case '?':
		case '-':
			return;

        // Since TAB would be added to the value, inercept it here and
        // emulate TAB-traversal to the next control instead
        case WXK_TAB:
            if (ev.ShiftDown())
            {
                _nameEntry->SetFocus();
            }
            else
            {
                _numPages->SetFocus();
            }
            return;

        // Check Uniqueness of the XData-Name.
		case WXK_RETURN:
		case WXK_NUMPAD_ENTER:
			checkXDataUniqueness();
			ev.Skip();
			break;

		default:
			ev.Skip();
			break;
		}
	}
	else if (ev.GetEventObject() == _nameEntry)
	{
		switch (ev.GetKeyCode())
		{
			// Forbidden key check
			case WXK_TAB:
                _xDataNameEntry->SetFocus();
				return; // forbid the tab
			default:
				ev.Skip();
				return;
		}
	}
    else if (ev.GetEventObject() == _numPages)
	{
		if (ev.GetKeyCode() != WXK_ESCAPE)
		{
			ev.Skip();
			return;
		}

		// Restore the old value.
		_numPages->SetValue(static_cast<int>(_xData->getNumPages()));
		return;
	}
    else if (ev.GetEventObject() == _guiEntry)
	{
		if (ev.GetKeyCode() != WXK_RETURN && ev.GetKeyCode() != WXK_NUMPAD_ENTER)
		{
			ev.Skip();
			return;
		}

		checkGuiLayout();
		return;
	}

	ev.Skip();
}

// Sets the text of a TextView identified by its widget enumerator and scrolls it to the end.
void ReadableEditorDialog::setTextViewAndScroll(wxTextCtrl* view, const std::string& text)
{
	view->SetValue(text);
	view->ShowPosition(view->GetLastPosition());
}

} // namespace ui
