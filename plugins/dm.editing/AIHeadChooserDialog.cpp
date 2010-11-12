#include "AIHeadChooserDialog.h"

#include "i18n.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "ieclass.h"

#include "gtkutil/TextColumn.h"
#include "gtkutil/MultiMonitor.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/RightAlignment.h"

#include <gtkmm/treeview.h>
#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/stock.h>
#include <gtkmm/textview.h>

namespace ui
{

	namespace
	{
		const char* const WINDOW_TITLE = N_("Choose AI Head");
	}

AIHeadChooserDialog::AIHeadChooserDialog() :
	gtkutil::BlockingTransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow()),
	_headStore(Gtk::ListStore::create(_columns)),
	_result(RESULT_CANCEL)
{
	_headsView = Gtk::manage(new Gtk::TreeView(_headStore));

	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 6));

	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	const Glib::RefPtr<Gtk::Window>& mainWindow = GlobalMainFrame().getTopLevelWindow();

	Gdk::Rectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(mainWindow);
	set_default_size(
		static_cast<int>(rect.get_width() * 0.7f), static_cast<int>(rect.get_height() * 0.6f)
	);

	// Allocate and setup the preview
	_preview = GlobalUIManager().createModelPreview();
	assert(_preview != NULL);

	_preview->setSize(static_cast<int>(rect.get_width() * 0.3f));

	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(false, 6));

	_headsView->set_headers_visible(false);

	_headsView->get_selection()->signal_changed().connect(
		sigc::mem_fun(*this, &AIHeadChooserDialog::onHeadSelectionChanged));

	// Head Name column
	_headsView->append_column(*Gtk::manage(new gtkutil::TextColumn("", _columns.name)));

	// Left: the treeview
	hbx->pack_start(*Gtk::manage(new gtkutil::ScrolledFrame(*_headsView)), true, true, 0);

	// Right: the preview and the description
	Gtk::VBox* vbox2 = Gtk::manage(new Gtk::VBox(false, 3));

	vbox2->pack_start(*_preview->getWidget(), true, true, 0);
	vbox2->pack_start(createDescriptionPanel(), false, false, 0);

	hbx->pack_start(*vbox2, false, false, 0);

	// Topmost: the tree plus preview
	vbox->pack_start(*hbx, true, true, 0);
	// Bottom: the button panel
	vbox->pack_start(createButtonPanel(), false, false, 0);

	add(*vbox);

	// Check if the liststore is populated
	findAvailableHeads();

	// Load the found heads into the GtkListStore
	populateHeadStore();
}

AIHeadChooserDialog::Result AIHeadChooserDialog::getResult()
{
	return _result;
}

void AIHeadChooserDialog::setSelectedHead(const std::string& headDef)
{
	_selectedHead = headDef;

	if (_selectedHead.empty())
	{
		_headsView->get_selection()->unselect_all();
		return;
	}

	// Lookup the model path in the treemodel
	if (!gtkutil::TreeModel::findAndSelectString(_headsView, _selectedHead, _columns.name))
	{
		_headsView->get_selection()->unselect_all();
	}
}

std::string AIHeadChooserDialog::getSelectedHead()
{
	return _selectedHead;
}

Gtk::Widget& AIHeadChooserDialog::createButtonPanel()
{
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(true, 6));

	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));
	_okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));

	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &AIHeadChooserDialog::onCancel));
	_okButton->signal_clicked().connect(sigc::mem_fun(*this, &AIHeadChooserDialog::onOK));

	hbx->pack_end(*_okButton, true, true, 0);
	hbx->pack_end(*cancelButton, true, true, 0);

	return *Gtk::manage(new gtkutil::RightAlignment(*hbx));
}

Gtk::Widget& AIHeadChooserDialog::createDescriptionPanel()
{
	// Create a GtkTextView
	_description = Gtk::manage(new Gtk::TextView);

	_description->set_wrap_mode(Gtk::WRAP_WORD);
	_description->set_editable(false);

	return *Gtk::manage(new gtkutil::ScrolledFrame(*_description));
}

void AIHeadChooserDialog::onCancel()
{
	_selectedHead = "";
	_result = RESULT_CANCEL;

	destroy();
}

void AIHeadChooserDialog::onOK()
{
	_result = RESULT_OK;

	// Done, just destroy the window
	destroy();
}

void AIHeadChooserDialog::_preDestroy()
{
	// Clear the model preview cache
	_preview->clear();

	BlockingTransientWindow::_preDestroy();
}

void AIHeadChooserDialog::_postShow()
{
	// Initialise the GL widget after the widgets have been shown
	_preview->initialisePreview();

	BlockingTransientWindow::_postShow();
}

void AIHeadChooserDialog::onHeadSelectionChanged()
{
	// Prepare to check for a selection
	Gtk::TreeModel::iterator iter = _headsView->get_selection()->get_selected();

	// Add button is enabled if there is a selection and it is not a folder.
	if (iter)
	{
		// Make the OK button active
		_okButton->set_sensitive(true);
		_description->set_sensitive(true);

		// Set the panel text with the usage information
		_selectedHead = Glib::ustring((*iter)[_columns.name]);

		// Lookup the IEntityClass instance
		IEntityClassPtr eclass = GlobalEntityClassManager().findClass(_selectedHead);

		if (eclass != NULL)
		{
			_preview->setModel(eclass->getAttribute("model").value);
			_preview->setSkin(eclass->getAttribute("skin").value);

			// Update the usage panel
			Glib::RefPtr<Gtk::TextBuffer> buf = _description->get_buffer();

			// Create the concatenated usage string
			std::string usage = "";
			EntityClassAttributeList usageAttrs = eclass->getAttributeList("editor_usage");

			for (EntityClassAttributeList::const_iterator i = usageAttrs.begin();
				 i != usageAttrs.end();
				 ++i)
			{
				// Add only explicit (non-inherited) usage strings
				if (!i->inherited) {
					if (!usage.empty())
						usage += std::string("\n") + i->value;
					else
						usage += i->value;
				}
			}

			buf->set_text(usage);
		}
	}
	else
	{
		_selectedHead = "";
		_preview->setModel("");

		_okButton->set_sensitive(false);
		_description->set_sensitive(false);
	}
}

void AIHeadChooserDialog::populateHeadStore()
{
	// Clear the head list to be safe
	_headStore->clear();

	for (HeadList::const_iterator i = _availableHeads.begin(); i != _availableHeads.end(); ++i)
	{
		// Add the entity to the list
		Gtk::TreeModel::Row row = *_headStore->append();
		row[_columns.name] = *i;
	}
}

namespace
{

class HeadEClassFinder :
	public EntityClassVisitor
{
	AIHeadChooserDialog::HeadList& _list;

public:
	HeadEClassFinder(AIHeadChooserDialog::HeadList& list) :
		_list(list)
	{}

	void visit(IEntityClassPtr eclass)
	{
		if (eclass->getAttribute("editor_head").value == "1")
		{
			_list.insert(eclass->getName());
		}
	}
};

} // namespace

void AIHeadChooserDialog::findAvailableHeads()
{
	if (!_availableHeads.empty())
	{
		return;
	}

	// Instantiate a finder class and traverse all eclasses
	HeadEClassFinder visitor(_availableHeads);
	GlobalEntityClassManager().forEach(visitor);
}

// Init static class member
AIHeadChooserDialog::HeadList AIHeadChooserDialog::_availableHeads;

} // namespace ui
