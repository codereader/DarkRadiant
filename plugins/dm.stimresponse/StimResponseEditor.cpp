#include "StimResponseEditor.h"

#include "iregistry.h"
#include "iundo.h"
#include "iscenegraph.h"
#include "itextstream.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "selectionlib.h"
#include "wxutil/dialog/MessageBox.h"
#include "string/string.h"

#include "i18n.h"
#include <iostream>

#include "StimEditor.h"
#include "ResponseEditor.h"

#include <wx/artprov.h>

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Stim/Response Editor");

	const std::string RKEY_ROOT = "user/ui/stimResponseEditor/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";

	const char* NO_ENTITY_ERROR = N_("A single entity must be selected to edit "
								  "Stim/Response properties.");
}

StimResponseEditor::StimResponseEditor() :
	DialogBase(_(WINDOW_TITLE)),
	//_notebook(new wxNotebook(this, wxID_ANY)),
	_entity(nullptr)
	//_stimEditor(new StimEditor(_notebook, _stimTypes)),
	//_responseEditor(new ResponseEditor(_notebook, _stimTypes)),
	//_customStimEditor(new CustomStimEditor(_notebook, _stimTypes))
{
	// Create the widgets
	populateWindow();

	// Connect the window position tracker
	_windowPosition.loadFromPath(RKEY_WINDOW_STATE);

	_windowPosition.connect(this);
	_windowPosition.applyPosition();
}

int StimResponseEditor::ShowModal()
{
	// Restore the position
	_windowPosition.applyPosition();

	// Reload all the stim types, the map might have changed
	_stimTypes.reload();

	// Scan the selection for entities
	rescanSelection();

	// Has the rescan found an entity (the pointer is non-NULL then)
	if (_entity != NULL)
	{
		// Show the last shown page
		_notebook->SetSelection(_lastShownPage);
	}

	int returnCode = DialogBase::ShowModal();

	if (returnCode == wxID_OK)
	{
		save();
	}

	_lastShownPage = _notebook->GetSelection();

	// Tell the position tracker to save the information
	_windowPosition.saveToPath(RKEY_WINDOW_STATE);

	return returnCode;
}

void StimResponseEditor::populateWindow()
{
	auto mainPanel = loadNamedPanel(this, "SREditorMainPanel");
	
	_notebook = findNamedObject<wxNotebook>(this, "SREditorNotebook");

	// Set up the list containing the stims and responses

	_stimEditor = new StimEditor(mainPanel, _stimTypes);

#if 0
	SetSizer(new wxBoxSizer(wxVERTICAL));

	_imageList.reset(new wxImageList(16, 16));
	_notebook->SetImageList(_imageList.get());

	// Stim Editor Page
	int imageId = _imageList->Add(
		wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + ICON_STIM + SUFFIX_EXTENSION));
	
	_notebook->AddPage(_stimEditor, _("Stims"), false, imageId);
	_stimPageNum = _notebook->FindPage(_stimEditor);

	// Response Editor Page
	imageId = _imageList->Add(
		wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + ICON_RESPONSE + SUFFIX_EXTENSION));
	
	_notebook->AddPage(_responseEditor, _("Responses"), false, imageId);
	_responsePageNum = _notebook->FindPage(_responseEditor);

	// Custom Stim Editor
	imageId = _imageList->Add(
		wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + ICON_CUSTOM_STIM));
	
	_notebook->AddPage(_customStimEditor, _("Custom Stims"), false, imageId);
	_customStimPageNum = _notebook->FindPage(_customStimEditor);
#endif
	_notebook->Connect(wxEVT_NOTEBOOK_PAGE_CHANGED, 
		wxBookCtrlEventHandler(StimResponseEditor::onPageChanged), nullptr, this);

	findNamedObject<wxButton>(this, "SREditorOkButton")->Bind(
		wxEVT_BUTTON, [this](wxCommandEvent& ev) { EndModal(wxID_OK); });
	findNamedObject<wxButton>(this, "SREditorCancelButton")->Bind(
		wxEVT_BUTTON, [this](wxCommandEvent& ev) { EndModal(wxID_CANCEL); });

	if (_lastShownPage == -1 && _notebook->GetPageCount() > 0)
	{
		_lastShownPage = -1;
	}

	Layout();
	Fit();
}

void StimResponseEditor::onPageChanged(wxBookCtrlEvent& ev)
{
	// The stim type list might have changed using the CustomStimEditor,
	// so let's update the controls when switching pages
	if (_stimEditor != nullptr)
	{
		_stimEditor->reloadStimTypes();
	}

	if (_responseEditor != nullptr)
	{
		_responseEditor->reloadStimTypes();
	}
}

void StimResponseEditor::rescanSelection()
{
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	_entity = nullptr;
	_srEntity.reset();
	_stimEditor->setEntity(_srEntity);
	_responseEditor->setEntity(_srEntity);
	//_customStimEditor->setEntity(_srEntity);

	if (info.entityCount == 1 && info.totalCount == 1)
	{
		// Get the entity instance
		const scene::INodePtr& node = GlobalSelectionSystem().ultimateSelected();

		_entity = Node_getEntity(node);

		_srEntity = SREntityPtr(new SREntity(_entity, _stimTypes));
		_stimEditor->setEntity(_srEntity);
		_responseEditor->setEntity(_srEntity);
		//_customStimEditor->setEntity(_srEntity);
	}

	if (_entity != NULL)
	{
		std::string title = _(WINDOW_TITLE);
		title += " (" + _entity->getKeyValue("name") + ")";
		SetTitle(title);
	}
	else
	{
		SetTitle(_(WINDOW_TITLE));
	}
}

void StimResponseEditor::save()
{
	// Consistency check can go here

	// Scoped undo object
	UndoableCommand command("editStimResponse");

	// Save the working set to the entity
	_srEntity->save(_entity);

	// Save the custom stim types to the storage entity
	_stimTypes.save();
}

bool StimResponseEditor::Destroy()
{
	// We experience crashes in Linux/GTK during dialog destruction when GTK+ 
	// apparently starts sending out the notebook page changed event right 
	// before removing the pages.
	_notebook->Disconnect(wxEVT_NOTEBOOK_PAGE_CHANGED, 
		wxBookCtrlEventHandler(StimResponseEditor::onPageChanged), nullptr, this);

	return wxutil::DialogBase::Destroy();
}

// Static command target
void StimResponseEditor::ShowDialog(const cmd::ArgumentList& args)
{
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	if (info.entityCount == 1 && info.totalCount == 1)
	{
		// Construct a new instance, this enters the main loop
		StimResponseEditor* editor = new StimResponseEditor;

		editor->ShowModal();
		editor->Destroy();
	}
	else
	{
		// Exactly one entity must be selected.
		wxutil::Messagebox::ShowError(_(NO_ENTITY_ERROR));
	}
}

int StimResponseEditor::_lastShownPage = -1;

} // namespace ui
