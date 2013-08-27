#include "SoundChooser.h"

#include "i18n.h"
#include "iuimanager.h"
#include "isound.h"
#include "imainframe.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/MultiMonitor.h"
#include "gtkutil/VFSTreePopulator.h"

#include <gtkmm/button.h>
#include <gtkmm/treeview.h>
#include <gtkmm/box.h>
#include <gtkmm/stock.h>

#include <boost/bind.hpp>

namespace ui
{

namespace
{
	const char* const SHADER_ICON = "icon_sound.png";
	const char* const FOLDER_ICON = "folder16.png";
}

// Constructor
SoundChooser::SoundChooser() :
	BlockingTransientWindow(_("Choose sound"), GlobalMainFrame().getTopLevelWindow()),
	_treeStore(Gtk::TreeStore::create(_columns)),
	_treeView(NULL),
	_preview(Gtk::manage(new SoundShaderPreview))
{
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	// Set the default size of the window
	Gdk::Rectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(GlobalMainFrame().getTopLevelWindow());
	set_default_size(rect.get_width() / 2, rect.get_height() / 2);

	// Main vbox
	Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 12));

    vbx->pack_start(createTreeView(), true, true, 0);
    vbx->pack_start(*_preview, false, false, 0);
    vbx->pack_start(createButtons(), false, false, 0);

    add(*vbx);
}

namespace
{

/**
 * Visitor class to enumerate sound shaders and add them to the tree store.
 */
class SoundShaderPopulator :
	public gtkutil::VFSTreePopulator,
	public gtkutil::VFSTreePopulator::Visitor
{
private:
	const SoundChooser::TreeColumns& _columns;
public:
	// Constructor
	SoundShaderPopulator(const Glib::RefPtr<Gtk::TreeStore>& treeStore,
						 const SoundChooser::TreeColumns& columns) :
		gtkutil::VFSTreePopulator(treeStore),
		_columns(columns)
	{}

    // Invoked for each sound shader
	void addShader(const ISoundShader& shader)
	{
		// Construct a "path" into the sound shader tree,
		// using the mod name as first folder level
		// angua: if there is a displayFolder present, put it between the mod name and the shader name
		std::string displayFolder = shader.getDisplayFolder();
		if (!displayFolder.empty())
		{
			addPath(shader.getModName() + "/" + displayFolder + "/" + shader.getName());
		}
		else
		{
			addPath(shader.getModName() + "/" + shader.getName());
		}
	}

	// Required visit function
	void visit(const Glib::RefPtr<Gtk::TreeStore>& store,
			   const Gtk::TreeModel::iterator& iter,
			   const std::string& path, bool isExplicit)
	{
		Gtk::TreeModel::Row row = *iter;

		// Get the display name by stripping off everything before the last slash
		std::string displayName = path.substr(path.rfind('/') + 1);

		// Fill in the column values
		row[_columns.displayName] = displayName;
		// angua: we need to remove mod name and displayfolder
		// it's not possible right now to have slashes in the shader name
		row[_columns.shaderName] = displayName;
		row[_columns.isFolder] = !isExplicit;
		row[_columns.icon] = GlobalUIManager().getLocalPixbuf(isExplicit ? SHADER_ICON : FOLDER_ICON);
	}
};


} // namespace

// Create the tree view
Gtk::Widget& SoundChooser::createTreeView()
{
	// Tree view with single text icon column
	_treeView = Gtk::manage(new Gtk::TreeView(_treeStore));

    // angua: Ensure sound shaders are sorted before giving them to the tree view
    gtkutil::TreeModel::applyFoldersFirstSortFunc(
        _treeStore, _columns.displayName, _columns.isFolder
    );


	_treeView->append_column(
		*Gtk::manage(new gtkutil::IconTextColumn(_("Soundshader"), _columns.displayName, _columns.icon))
	);

	_treeSelection = _treeView->get_selection();
	_treeSelection->signal_changed().connect(sigc::mem_fun(*this, &SoundChooser::_onSelectionChange));

	// Populate the tree store with sound shaders, using a VFS tree populator
	SoundShaderPopulator pop(_treeStore, _columns);

	// Visit all sound shaders and collect them for later insertion
	GlobalSoundManager().forEachShader(
        boost::bind(&SoundShaderPopulator::addShader, boost::ref(pop), _1)
    );

	// Let the populator iterate over all collected elements
	// and insert them in the treestore
	pop.forEachNode(pop);

	return *Gtk::manage(new gtkutil::ScrolledFrame(*_treeView));
}

// Create buttons panel
Gtk::Widget& SoundChooser::createButtons()
{
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(false, 6));

	Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));

	okButton->signal_clicked().connect(sigc::mem_fun(*this, &SoundChooser::_onOK));
	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &SoundChooser::_onCancel));

	hbx->pack_end(*okButton, false, false, 0);
	hbx->pack_end(*cancelButton, false, false, 0);

	return *Gtk::manage(new gtkutil::RightAlignment(*hbx));
}

void SoundChooser::_onDeleteEvent()
{
	_selectedShader.clear();

	BlockingTransientWindow::_onDeleteEvent();
}

const std::string& SoundChooser::getSelectedShader() const
{
	return _selectedShader;
}

// Set the selected sound shader, and focuses the treeview to the new selection
void SoundChooser::setSelectedShader(const std::string& shader)
{
	if (!gtkutil::TreeModel::findAndSelectString(_treeView, shader, _columns.shaderName))
	{
		_treeSelection->unselect_all();
	}
}

void SoundChooser::_onOK()
{
	destroy();
}

void SoundChooser::_onCancel()
{
	_selectedShader.clear();
	destroy();
}

void SoundChooser::_onSelectionChange()
{
	Gtk::TreeModel::iterator iter = _treeSelection->get_selected();

	if (iter)
	{
		Gtk::TreeModel::Row row = *iter;

		bool isFolder = row[_columns.isFolder];

		_selectedShader = isFolder ? "" : std::string(row[_columns.shaderName]);
	}
	else
	{
		_selectedShader.clear();
	}

	// Notify the preview widget about the change
	_preview->setSoundShader(_selectedShader);
}

} // namespace
