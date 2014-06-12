#include "StimResponseEditor.h"

#include "iregistry.h"
#include "iundo.h"
#include "iscenegraph.h"
#include "itextstream.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "selectionlib.h"
#include "gtkutil/dialog/MessageBox.h"
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
	_notebook(new wxNotebook(this, wxID_ANY)),
	_entity(NULL),
	_stimEditor(new StimEditor(_notebook, _stimTypes)),
	_responseEditor(new ResponseEditor(_notebook, _stimTypes)),
	_customStimEditor(new CustomStimEditor(_notebook, _stimTypes))
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

	// Pack everything into the main window
	GetSizer()->Add(_notebook, 1, wxEXPAND | wxALL, 12);
	GetSizer()->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxALL, 12);

	if (_lastShownPage == -1)
	{
		_lastShownPage = _stimPageNum;
	}

	Layout();
	Fit();
}

void StimResponseEditor::rescanSelection()
{
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	_entity = NULL;
	_srEntity = SREntityPtr();
	_stimEditor->setEntity(_srEntity);
	_responseEditor->setEntity(_srEntity);
	_customStimEditor->setEntity(_srEntity);

	if (info.entityCount == 1 && info.totalCount == 1)
	{
		// Get the entity instance
		const scene::INodePtr& node = GlobalSelectionSystem().ultimateSelected();

		_entity = Node_getEntity(node);

		_srEntity = SREntityPtr(new SREntity(_entity, _stimTypes));
		_stimEditor->setEntity(_srEntity);
		_responseEditor->setEntity(_srEntity);
		_customStimEditor->setEntity(_srEntity);
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
