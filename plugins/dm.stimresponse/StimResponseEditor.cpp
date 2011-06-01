#include "StimResponseEditor.h"

#include "iregistry.h"
#include "iundo.h"
#include "iscenegraph.h"
#include "itextstream.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "ieventmanager.h"
#include "selectionlib.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/RightAlignedLabel.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/IconTextButton.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/StockIconMenuItem.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/dialog/MessageBox.h"
#include "string/string.h"

#include <gtkmm/notebook.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>
#include <gtkmm/stock.h>
#include <gtkmm/label.h>
#include <gtkmm/box.h>

#include "i18n.h"
#include <iostream>

#include "StimEditor.h"
#include "ResponseEditor.h"

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
	gtkutil::BlockingTransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow()),
	_entity(NULL),
	_stimEditor(Gtk::manage(new StimEditor(_stimTypes))),
	_responseEditor(Gtk::manage(new ResponseEditor(getRefPtr(), _stimTypes))),
	_customStimEditor(Gtk::manage(new CustomStimEditor(_stimTypes)))
{
	// Set the default border width in accordance to the HIG
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	signal_key_press_event().connect(sigc::mem_fun(*this, &StimResponseEditor::onWindowKeyPress), false);

	// Create the widgets
	populateWindow();

	// Connect the window position tracker
	_windowPosition.loadFromPath(RKEY_WINDOW_STATE);

	_windowPosition.connect(this);
	_windowPosition.applyPosition();

	// Show the dialog, this enters the gtk main loop
	show();
}

void StimResponseEditor::_preHide()
{
	_lastShownPage = _notebook->get_current_page();

	// Tell the position tracker to save the information
	_windowPosition.saveToPath(RKEY_WINDOW_STATE);
}

void StimResponseEditor::_preShow()
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
		// Now show the dialog window again
		show_all();

		// Show the last shown page
		_notebook->set_current_page(_lastShownPage);
	}
}

void StimResponseEditor::populateWindow()
{
	// Create the overall vbox
	_dialogVBox = Gtk::manage(new Gtk::VBox(false, 12));
	add(*_dialogVBox);

	// Create the notebook and add it to the vbox
	_notebook = Gtk::manage(new Gtk::Notebook);
	_dialogVBox->pack_start(*_notebook, true, true, 0);

	// The tab label items (icon + label)
	Gtk::HBox* stimLabelHBox = Gtk::manage(new Gtk::HBox(false, 0));
	stimLabelHBox->pack_start(
		*Gtk::manage(new Gtk::Image(
			GlobalUIManager().getLocalPixbufWithMask(ICON_STIM + SUFFIX_EXTENSION))),
    	false, false, 3
    );
	stimLabelHBox->pack_start(*Gtk::manage(new Gtk::Label(_("Stims"))), false, false, 3);

	Gtk::HBox* responseLabelHBox = Gtk::manage(new Gtk::HBox(false, 0));
	responseLabelHBox->pack_start(
		*Gtk::manage(new Gtk::Image(
			GlobalUIManager().getLocalPixbufWithMask(ICON_RESPONSE + SUFFIX_EXTENSION))),
    	false, false, 3
    );
	responseLabelHBox->pack_start(*Gtk::manage(new Gtk::Label(_("Responses"))), false, false, 3);

	Gtk::HBox* customLabelHBox = Gtk::manage(new Gtk::HBox(false, 0));
	customLabelHBox->pack_start(
		*Gtk::manage(new Gtk::Image(
			GlobalUIManager().getLocalPixbufWithMask(ICON_CUSTOM_STIM))),
    	false, false, 3
    );
	customLabelHBox->pack_start(*Gtk::manage(new Gtk::Label(_("Custom Stims"))), false, false, 3);

	// Show the widgets before using them as label, they won't appear otherwise
	stimLabelHBox->show_all();
	responseLabelHBox->show_all();
	customLabelHBox->show_all();

	// Cast the helper class to a widget and add it to the notebook page
	_stimPageNum = _notebook->append_page(*_stimEditor, *stimLabelHBox);
	_responsePageNum = _notebook->append_page(*_responseEditor, *responseLabelHBox);
	_customStimPageNum = _notebook->append_page(*_customStimEditor, *customLabelHBox);

	if (_lastShownPage == -1)
	{
		_lastShownPage = _stimPageNum;
	}

	// Pack in dialog buttons
	_dialogVBox->pack_start(createButtons(), false, false, 0);
}

// Lower dialog buttons
Gtk::Widget& StimResponseEditor::createButtons()
{
	Gtk::HBox* buttonHBox = Gtk::manage(new Gtk::HBox(true, 12));

	// Save button
	Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
	okButton->signal_clicked().connect(sigc::mem_fun(*this, &StimResponseEditor::onSave));

	// Close Button
	_closeButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));
	_closeButton->signal_clicked().connect(sigc::mem_fun(*this, &StimResponseEditor::onClose));

	buttonHBox->pack_end(*okButton, true, true, 0);
	buttonHBox->pack_end(*_closeButton, true, true, 0);

	return *Gtk::manage(new gtkutil::RightAlignment(*buttonHBox));
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
		set_title(title);
	}
	else
	{
		set_title(_(WINDOW_TITLE));
	}

	_dialogVBox->set_sensitive(_entity != NULL);
	_closeButton->set_sensitive(true);
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

void StimResponseEditor::onSave()
{
	save();
	destroy();
}

void StimResponseEditor::onClose()
{
	destroy();
}

bool StimResponseEditor::onWindowKeyPress(GdkEventKey* ev)
{
	if (ev->keyval == GDK_Escape)
	{
		destroy();
		// Catch this keyevent, don't propagate
		return true;
	}

	// Propagate further
	return false;
}

// Static command target
void StimResponseEditor::showDialog(const cmd::ArgumentList& args) {
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	if (info.entityCount == 1 && info.totalCount == 1)
	{
		// Construct a new instance, this enters the main loop
		StimResponseEditor _editor;
	}
	else
	{
		// Exactly one entity must be selected.
		gtkutil::MessageBox::ShowError(_(NO_ENTITY_ERROR), GlobalMainFrame().getTopLevelWindow());
	}
}

int StimResponseEditor::_lastShownPage = -1;

} // namespace ui
