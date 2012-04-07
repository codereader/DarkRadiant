#include "AIVocalSetChooserDialog.h"

#include "i18n.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "ieclass.h"
#include "isound.h"

#include "eclass.h"

#include "gtkutil/TextColumn.h"
#include "gtkutil/MultiMonitor.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"

#include <gtkmm/treeview.h>
#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/stock.h>
#include <gtkmm/textview.h>

namespace ui
{

	namespace
	{
		const char* const WINDOW_TITLE = N_("Choose AI Vocal Set");
	}

AIVocalSetChooserDialog::AIVocalSetChooserDialog() :
	gtkutil::BlockingTransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow()),
	_setStore(Gtk::ListStore::create(_columns)),
	_result(RESULT_CANCEL),
	_preview(NULL)
{
	if (module::GlobalModuleRegistry().moduleExists(MODULE_SOUNDMANAGER))
	{
		_preview = Gtk::manage(new AIVocalSetPreview);
	}

	_setView = Gtk::manage(new Gtk::TreeView(_setStore));

	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 6));

	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	const Glib::RefPtr<Gtk::Window>& mainWindow = GlobalMainFrame().getTopLevelWindow();

	Gdk::Rectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(mainWindow);
	set_default_size(
		static_cast<int>(rect.get_width() * 0.7f), static_cast<int>(rect.get_height() * 0.6f)
	);

	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(false, 6));

	_setView->set_headers_visible(false);

	_setView->get_selection()->signal_changed().connect(
		sigc::mem_fun(*this, &AIVocalSetChooserDialog::onSetSelectionChanged));

	// Head Name column
	_setView->append_column(*Gtk::manage(new gtkutil::TextColumn("", _columns.name)));

	// Left: the treeview
	Gtk::VBox* vbox1 = Gtk::manage(new Gtk::VBox(false, 3));
	hbx->pack_start(*vbox1, true, true, 0);

	vbox1->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(_("<b>Available Sets:</b>"))), false, false, 0);
	vbox1->pack_start(*Gtk::manage(new gtkutil::ScrolledFrame(*_setView)), true, true, 0);

	// Right: the description
	Gtk::VBox* vbox2 = Gtk::manage(new Gtk::VBox(false, 3));

	vbox2->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(_("<b>Description:</b>"))), false, false, 0);

	Gtk::Widget& descPanel = createDescriptionPanel();
	vbox2->pack_start(descPanel, true, true, 0);
	descPanel.set_size_request(static_cast<int>(rect.get_width()*0.2f), -1);

	// Right: the preview control panel
	if (_preview != NULL)
	{
		vbox2->pack_start(*_preview, false, false, 0);
	}

	hbx->pack_start(*vbox2, false, false, 0);

	// Topmost: the tree plus description
	vbox->pack_start(*hbx, true, true, 0);
	// Bottom: the button panel
	vbox->pack_start(createButtonPanel(), false, false, 0);

	add(*vbox);

	// Check if the liststore is populated
	findAvailableSets();

	// Load the found sets into the GtkListStore
	populateSetStore();
}

AIVocalSetChooserDialog::Result AIVocalSetChooserDialog::getResult()
{
	return _result;
}

void AIVocalSetChooserDialog::setSelectedVocalSet(const std::string& setName)
{
	_selectedSet = setName;

	if (_selectedSet.empty())
	{
		_setView->get_selection()->unselect_all();
		return;
	}

	// Lookup the model path in the treemodel
	if (!gtkutil::TreeModel::findAndSelectString(_setView, _selectedSet, _columns.name))
	{
		_setView->get_selection()->unselect_all();
	}
}

std::string AIVocalSetChooserDialog::getSelectedVocalSet()
{
	return _selectedSet;
}

Gtk::Widget& AIVocalSetChooserDialog::createButtonPanel()
{
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(true, 6));

	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));
	_okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));

	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &AIVocalSetChooserDialog::onCancel));
	_okButton->signal_clicked().connect(sigc::mem_fun(*this, &AIVocalSetChooserDialog::onOK));

	hbx->pack_end(*_okButton, true, true, 0);
	hbx->pack_end(*cancelButton, true, true, 0);

	return *Gtk::manage(new gtkutil::RightAlignment(*hbx));
}

Gtk::Widget& AIVocalSetChooserDialog::createDescriptionPanel()
{
	// Create a GtkTextView
	_description = Gtk::manage(new Gtk::TextView);

	_description->set_wrap_mode(Gtk::WRAP_WORD);
	_description->set_editable(false);

	return *Gtk::manage(new gtkutil::ScrolledFrame(*_description));
}

void AIVocalSetChooserDialog::onCancel()
{
	_selectedSet = "";
	_result = RESULT_CANCEL;

	destroy();
}

void AIVocalSetChooserDialog::onOK()
{
	_result = RESULT_OK;

	// Done, just destroy the window
	destroy();
}

void AIVocalSetChooserDialog::onSetSelectionChanged()
{
	// Prepare to check for a selection
	Gtk::TreeModel::iterator iter = _setView->get_selection()->get_selected();

	// Add button is enabled if there is a selection and it is not a folder.
	if (iter)
	{
		// Make the OK button active
		_okButton->set_sensitive(true);
		_description->set_sensitive(true);

		// Set the panel text with the usage information
		_selectedSet = Glib::ustring((*iter)[_columns.name]);

		// Lookup the IEntityClass instance
		IEntityClassPtr ecls = GlobalEntityClassManager().findClass(_selectedSet);
		if (ecls)
		{
			// Update the preview pane
			if (_preview != NULL)
			{
				_preview->setVocalSetEclass(ecls);
			}

			// Update the usage panel
			_description->get_buffer()->set_text(eclass::getUsage(*ecls));
		}
	}
	else
	{
		_selectedSet = "";

		if (_preview != NULL)
		{
			_preview->setVocalSetEclass(IEntityClassPtr());
		}

		_okButton->set_sensitive(false);
		_description->set_sensitive(false);
	}
}

void AIVocalSetChooserDialog::populateSetStore()
{
	// Clear the head list to be safe
	_setStore->clear();

	for (SetList::const_iterator i = _availableSets.begin(); i != _availableSets.end(); ++i)
	{
		// Add the entity to the list
		Gtk::TreeModel::Row row = *_setStore->append();

		row[_columns.name] = *i;
	}
}

namespace
{

class VocalSetEClassFinder :
	public EntityClassVisitor
{
	AIVocalSetChooserDialog::SetList& _list;

public:
	VocalSetEClassFinder(AIVocalSetChooserDialog::SetList& list) :
		_list(list)
	{}

	void visit(const IEntityClassPtr& eclass)
	{
		if (eclass->getAttribute("editor_vocal_set").getValue() == "1")
		{
			_list.insert(eclass->getName());
		}
	}
};

} // namespace

void AIVocalSetChooserDialog::findAvailableSets()
{
	if (!_availableSets.empty())
	{
		return;
	}

	// Instantiate a finder class and traverse all eclasses
	VocalSetEClassFinder visitor(_availableSets);
	GlobalEntityClassManager().forEachEntityClass(visitor);
}

// Init static class member
AIVocalSetChooserDialog::SetList AIVocalSetChooserDialog::_availableSets;

} // namespace ui
