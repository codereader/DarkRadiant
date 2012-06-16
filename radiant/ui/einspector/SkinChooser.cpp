#include "SkinChooser.h"

#include "i18n.h"
#include "iuimanager.h"
#include "imainframe.h"
#include "modelskin.h"

#include "gtkutil/RightAlignment.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/VFSTreePopulator.h"
#include "gtkutil/MultiMonitor.h"

#include <gtkmm/box.h>
#include <gtkmm/treeview.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>

namespace ui
{

/* CONSTANTS */

namespace
{
	const char* FOLDER_ICON = "folder16.png";
	const char* SKIN_ICON = "skin16.png";

	const char* const WINDOW_TITLE = N_("Choose Skin");
}

// Constructor
SkinChooser::SkinChooser() :
	gtkutil::BlockingTransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow()),
	_lastSkin(""),
	_preview(new gtkutil::ModelPreview())
{
	set_border_width(6);

	// Set the default size of the window
	Gdk::Rectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(GlobalMainFrame().getTopLevelWindow());
	int w = rect.get_width();

	// Main vbox
	Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 6));

	// HBox containing tree view and preview
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(false, 12));

	hbx->pack_start(createTreeView(w / 5), true, true, 0);
	hbx->pack_start(createPreview(w / 3), false, false, 0);

	vbx->pack_start(*hbx, true, true, 0);
	vbx->pack_end(createButtons(), false, false, 0);

	add(*vbx);
}

SkinChooser& SkinChooser::Instance()
{
	SkinChooserPtr& instancePtr = InstancePtr();

	if (instancePtr == NULL)
	{
		// Not yet instantiated, do it now
		instancePtr.reset(new SkinChooser);

		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().signal_radiantShutdown().connect(
            sigc::mem_fun(*instancePtr, &SkinChooser::onRadiantShutdown)
        );
	}

	return *instancePtr;
}

SkinChooserPtr& SkinChooser::InstancePtr()
{
	static SkinChooserPtr _instancePtr;
	return _instancePtr;
}

Gtk::Widget& SkinChooser::createTreeView(int width)
{
	// Create the treestore
	_treeStore = Gtk::TreeStore::create(_columns);

	// Create the tree view
	_treeView = Gtk::manage(new Gtk::TreeView(_treeStore));
	_treeView->set_headers_visible(false);

	// Single column to display the skin name
	_treeView->append_column(*Gtk::manage(
		new gtkutil::IconTextColumn(_("Skin"), _columns.displayName, _columns.icon))
	);

	// Connect up selection changed callback
	_selection = _treeView->get_selection();
	_selection->signal_changed().connect(sigc::mem_fun(*this, &SkinChooser::_onSelChanged));

	// Pack treeview into a ScrolledFrame and return
	_treeView->set_size_request(width, -1);

	return *Gtk::manage(new gtkutil::ScrolledFrame(*_treeView));
}

// Create the model preview
Gtk::Widget& SkinChooser::createPreview(int size)
{
	_preview->setSize(size, size);

	return *_preview;
}

// Create the buttons panel
Gtk::Widget& SkinChooser::createButtons()
{
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(true, 6));

	Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));

	okButton->signal_clicked().connect(sigc::mem_fun(*this, &SkinChooser::_onOK));
	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &SkinChooser::_onCancel));

	hbx->pack_end(*okButton, true, true, 0);
	hbx->pack_end(*cancelButton, true, true, 0);

	return *Gtk::manage(new gtkutil::RightAlignment(*hbx));
}

void SkinChooser::_postShow()
{
	// Initialise the GL widget after the widgets have been shown
	_preview->initialisePreview();

	// Call the base class, will enter main loop
	BlockingTransientWindow::_postShow();
}

// Show the dialog and block for a selection
std::string SkinChooser::showAndBlock(const std::string& model,
									  const std::string& prev)
{
	// Set the model and previous skin, then populate the skins
	_model = model;
	_prevSkin = prev;
	populateSkins();

	// Display the model in the window title
	set_title(std::string(_(WINDOW_TITLE)) + ": " + _model);

	// Show the dialog and block
	show();

	// Hide the dialog and return the selection
	_preview->setModel(""); // release model

	return _lastSkin;
}

namespace
{

/*
 * Visitor class to fill in column data for the skins tree.
 */
class SkinTreeVisitor
: public gtkutil::VFSTreePopulator::Visitor
{
private:
	const SkinChooser::TreeColumns& _columns;

public:
	SkinTreeVisitor(const SkinChooser::TreeColumns& columns) :
		_columns(columns)
	{}

    virtual ~SkinTreeVisitor() {}

	// Required visit function
	void visit(const Glib::RefPtr<Gtk::TreeStore>& store,
			   const Gtk::TreeModel::iterator& iter,
			   const std::string& path,
			   bool isExplicit)
	{
		// Get the display path, everything after rightmost slash
		std::string displayPath = path.substr(path.rfind("/") + 1);

		Gtk::TreeModel::Row row = *iter;

		row[_columns.displayName] = displayPath;
		row[_columns.fullName] = path;

		// Get the icon, either folder or skin
		row[_columns.icon] =
			GlobalUIManager().getLocalPixbuf(isExplicit ? SKIN_ICON : FOLDER_ICON);
	}
};

} // namespace

// Populate the list of skins
void SkinChooser::populateSkins()
{
	// Clear the treestore
	_treeStore->clear();

	// Add the "Matching skins" toplevel node
	Gtk::TreeModel::iterator matchingSkins = _treeStore->append();
	Gtk::TreeModel::Row row = *matchingSkins;

	row[_columns.displayName] = _("Matching skins");
	row[_columns.fullName] = "";
	row[_columns.icon] = GlobalUIManager().getLocalPixbuf(FOLDER_ICON);

	// Get the skins for the associated model, and add them as matching skins
	const StringList& matchList = GlobalModelSkinCache().getSkinsForModel(_model);

	for (StringList::const_iterator i = matchList.begin();
		 i != matchList.end();
		 ++i)
	{
		Gtk::TreeModel::Row skinRow = *_treeStore->append(matchingSkins->children());

		skinRow[_columns.displayName] = *i;
		skinRow[_columns.fullName] = *i;
		skinRow[_columns.icon] = GlobalUIManager().getLocalPixbuf(SKIN_ICON);
	}

	// Add "All skins" toplevel node
	Gtk::TreeModel::iterator allSkins = _treeStore->append();
	row = *allSkins;

	row[_columns.displayName] = _("All skins");
	row[_columns.fullName] = "";
	row[_columns.icon] = GlobalUIManager().getLocalPixbuf(FOLDER_ICON);

	// Get the list of skins for the model
	const StringList& skins = GlobalModelSkinCache().getAllSkins();

	// Create a TreePopulator for the tree store and pass in each of the
	// skin names.
	gtkutil::VFSTreePopulator pop(_treeStore, allSkins);

	for (StringList::const_iterator i = skins.begin();
		 i != skins.end();
		 ++i)
	{
		pop.addPath(*i);
	}

	// Visit the tree populator in order to fill in the column data
	SkinTreeVisitor visitor(_columns);
	pop.forEachNode(visitor);
}

std::string SkinChooser::getSelectedSkin()
{
	// Get the selected skin
	Gtk::TreeModel::iterator iter = _selection->get_selected();

	if (iter)
	{
		return Glib::ustring((*iter)[_columns.fullName]);
	}
	else
	{
		return "";
	}
}

// Static method to display singleton instance and choose a skin
std::string SkinChooser::chooseSkin(const std::string& model,
									const std::string& prev)
{
	// Show and block the instance, returning the selected skin
	return Instance().showAndBlock(model, prev);
}

void SkinChooser::onRadiantShutdown()
{
	rMessage() << "SkinChooser shutting down." << std::endl;

	_preview.reset();

	InstancePtr().reset();
}

void SkinChooser::_onDeleteEvent()
{
	_lastSkin = _prevSkin;

	hide(); // just hide, don't call base class which might delete this dialog
}

void SkinChooser::_onOK() {

	// Get the selected skin
	_lastSkin = getSelectedSkin();

	hide(); // break main loop
}

void SkinChooser::_onCancel()
{
	// Clear the last skin and quit the main loop
	_lastSkin = _prevSkin;

	hide();
}

void SkinChooser::_onSelChanged()
{
	// Set the model preview to show the model with the selected skin
	_preview->setModel(_model);
	_preview->setSkin(getSelectedSkin());
}

} // namespace
