#pragma once

#include "icommandsystem.h"

#include "wxutil/dialog/DialogBase.h"
#include "ReadableGuiView.h"
#include <map>
#include <memory>
#include "XDataLoader.h"
#include "string/string.h"
#include "wxutil/XmlResourceBasedWidget.h"

class Entity;

class wxTextCtrl;
class wxSpinCtrl;
class wxRadioButton;
class wxStaticText;

namespace ui
{

namespace
{
	const std::string RKEY_READABLES_STORAGE_FOLDER = "user/ui/gui/storageFolder";
	const std::string RKEY_READABLES_CUSTOM_FOLDER = "user/ui/gui/customFolder";
}

///////////////////////////// ReadableEditorDialog:
// The main dialog of the Readable Editor, which implements most editing features.
class ReadableEditorDialog :
	public wxutil::DialogBase,
	private wxutil::XmlResourceBasedWidget
{
private:
	gui::ReadableGuiView* _guiView;

	// The entity we're working with
	Entity* _entity;

	// The XData-definition of the entity
	XData::XDataPtr _xData;

	// The filename of the XData definition
	std::string _xdFilename;

	// The XData filename derived from the map-name.
	std::string _mapBasedFilename;

	// The xData loader
	XData::XDataLoaderPtr _xdLoader;

	// The index of the current page.
	std::size_t _currentPageIndex;

	// Has the XData name been specified?
	bool _xdNameSpecified;

	// Tells the program whether checkGuiLayout() is already running.
	bool _runningGuiLayoutCheck;

	// Tells the program whether checkXDataUniqueness is already running.
	bool _runningXDataUniquenessCheck;

	// Tells the exporter whether to use the _mapBasedFilename (true) or not.
	bool _useDefaultFilename;

	// Prevents saving races.
	bool _saveInProgress;

	wxTextCtrl* _nameEntry;
	wxTextCtrl* _xDataNameEntry;

	wxSpinCtrl* _numPages;

	wxRadioButton* _oneSidedButton;
	wxRadioButton* _twoSidedButton;
	wxTextCtrl* _pageTurnEntry;
	wxStaticText* _curPageDisplay;
	wxTextCtrl* _guiEntry;

	wxStaticText* _pageLeftLabel;
	wxStaticText* _pageRightLabel;

	wxTextCtrl* _textViewTitle;
	wxTextCtrl* _textViewRightTitle;
	wxTextCtrl* _textViewBody;
	wxTextCtrl* _textViewRightBody;

	std::unique_ptr<wxMenu> _insertMenu;
	std::unique_ptr<wxMenu> _deleteMenu;
	std::unique_ptr<wxMenu> _appendMenu;
	std::unique_ptr<wxMenu> _prependMenu;
	std::unique_ptr<wxMenu> _toolsMenu;

public:
	// Pass the working entity to the constructor
	ReadableEditorDialog(Entity* entity);

	static void RunDialog(const cmd::ArgumentList& args);

	// override DialogBase
	int ShowModal();

	// Switch between the editing modes
	void useOneSidedEditing();
	void useTwoSidedEditing();

	// Updates the GUI preview (text updates). Public so that XDataSelector and GuiSelector can access it.
	// Uses the current data in the readable editor for updating or imports XData/guis by the passed strings.
	// This Method can create error-messages. For that reason a parent window can be specified. If Null the Readable
	// Editor Dialog is parent.
	void updateGuiView(wxWindow* parent = NULL,
					   const std::string& guiPath = "",
					   const std::string& xDataName = "",
					   const std::string& xDataPath = "");

    // Sets some GUI state variables needed for rendering and sets the time for script execution
    void initGuiState(const gui::IGuiPtr& gui);

	// shows the XData import Summary
	void showXdImportSummary();

private:
	// Save all settings on the entity and exports xdata.
	bool save();

	// Constructs the storage path for the current XData definition. Used by save-method and for the window-title.
	std::string constructStoragePath();

	// Refreshes the window title based on the storagepath.
	void refreshWindowTitle();

	// Retrieves information from the entity and imports xdata. If the user cancels, the window is destroyed in _postshow
	bool initControlsFromEntity();

	// _show_ TwoSided editing-interface.
	void toggleTwoSidedEditingInterface(bool show);

	// shows the gui import Summary
	void showGuiImportSummary();

	// toggles the layout of the xdata object and updates the interface.
	void toggleLayout();

	// Stores the contents of the current page inside the xData-object.
	void storeCurrentPage();

	// Stores the contents of all controls in the xData-object.
	void storeXData();

	// Updates the page related inputs and the preview renderer. Also adds default guis.
	// Warning: Contents are overwritten. storeCurrentPage() should be called beforehand.
	void showPage(std::size_t pageIndex);

	// Populates the controls with the information in the xdata object. Adds a default snd_page_turn, if not defined.
	void populateControlsFromXData();

	// Checks whether the chosen XData name already exists. If it does a popup will ask the user whether it should be imported.
	void checkXDataUniqueness();

	// Checks whether the specified gui definition matches the pagelayout. Returns FALSE on success.
	void checkGuiLayout();

	void handleNumberOfPagesChanged();

	// Deleting and inserting of pages.
	void insertPage();
	void deletePage();

	// Inserts a page on the right or left side of the current page.
	void insertSide(bool rightSide);

	// Deletes the left or right side of the current page.
	void deleteSide(bool rightSide);

	// Ui Creation:
	void setupGeneralPropertiesInterface();
	void setupPageRelatedInterface();
	void setupButtonPanel();
	void createMenus();

	// Callback methods for Signals:
	void onCancel(wxCommandEvent& ev);
	void onSave(wxCommandEvent& ev);
	void onSaveClose(wxCommandEvent& ev);

	void onBrowseXd(wxCommandEvent& ev);
	void onBrowseGui(wxCommandEvent& ev);
	void onFirstPage(wxCommandEvent& ev);
	void onPrevPage(wxCommandEvent& ev);
	void onNextPage(wxCommandEvent& ev);
	void onLastPage(wxCommandEvent& ev);

	void onInsert(wxCommandEvent& ev);
	void onDelete(wxCommandEvent& ev);

	void onNumPagesChanged(wxSpinEvent& ev);
	void onToolsClicked(wxCommandEvent& ev);
	void onMenuItemClick(wxCommandEvent& ev);
	void onOneSided(wxCommandEvent& ev);
	void onTwoSided(wxCommandEvent& ev);

	// Callback methods for Events:
	void onFocusOut(wxFocusEvent& ev);
    void onChar(wxKeyEvent& ev);
	void onTextChanged(wxCommandEvent& ev);

	// Helper Methods:

	void showDuplicateDefinitions();

	// Sets the text of a TextView identified by its widget enumerator and scrolls it to the end.
	void setTextViewAndScroll(wxTextCtrl* view, const std::string& text);
};

} // namespace ui
