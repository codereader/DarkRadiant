#include "ShaderInfoTab.h"

#include "i18n.h"

#include "string/string.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/LeftAlignedLabel.h"

#include <gtkmm/treeview.h>
#include <gtkmm/box.h>
#include <gtkmm/table.h>
#include <boost/bind.hpp>

#include "selection/algorithm/Shader.h"

namespace ui
{
	namespace
	{
		const char* const TAB_NAME = N_("Shaders");
		const std::string TAB_ICON("icon_texture.png");
		const char* const SELECT_ITEMS = N_("Select elements using this shader");
		const char* const DESELECT_ITEMS = N_("Deselect elements using this shader");
	}

ShaderInfoTab::ShaderInfoTab() :
	_widget(Gtk::manage(new Gtk::VBox(false, 6))),
	_shaderCount(Gtk::manage(new gtkutil::LeftAlignedLabel(""))),
	_listStore(Gtk::ListStore::create(_columns)),
	_treeView(Gtk::manage(new Gtk::TreeView(_listStore))),
	_popupMenu(_treeView)
{
	// Create all the widgets
	construct();

	_popupMenu.addItem(
		Gtk::manage(new Gtk::MenuItem(_(SELECT_ITEMS))),
		boost::bind(&ShaderInfoTab::_onSelectItems, this, true),
		boost::bind(&ShaderInfoTab::_testSelectItems, this)
	);
	_popupMenu.addItem(
		Gtk::manage(new Gtk::MenuItem(_(DESELECT_ITEMS))),
		boost::bind(&ShaderInfoTab::_onSelectItems, this, false),
		boost::bind(&ShaderInfoTab::_testSelectItems, this)
	);
}

Gtk::Widget& ShaderInfoTab::getWidget()
{
	return *_widget;
}

std::string ShaderInfoTab::getLabel()
{
	return _(TAB_NAME);
}

std::string ShaderInfoTab::getIconName()
{
	return TAB_ICON;
}

void ShaderInfoTab::construct()
{
	// Setup the outer space of the vbox
	_widget->set_border_width(12);

	// Setup the treeview and pack two columns into it
	_treeView->set_headers_clickable(true);

	Gtk::TreeViewColumn* shaderCol = Gtk::manage(new gtkutil::TextColumn(_("Shader"), _columns.shader));
	shaderCol->set_sort_column(_columns.shader);

	Gtk::TreeViewColumn* faceCountCol = Gtk::manage(new gtkutil::TextColumn(_("Faces"), _columns.faceCount));
	faceCountCol->set_sort_column(_columns.faceCount);

	Gtk::TreeViewColumn* patchCountCol = Gtk::manage(new gtkutil::TextColumn(_("Patches"), _columns.patchCount));
	patchCountCol->set_sort_column(_columns.patchCount);

	_treeView->append_column(*shaderCol);
	_treeView->append_column(*faceCountCol);
	_treeView->append_column(*patchCountCol);

    _widget->pack_start(*Gtk::manage(new gtkutil::ScrolledFrame(*_treeView)), true, true, 0);

    // Populate the liststore with the entity count information
    for (map::ShaderBreakdown::Map::const_iterator i = _shaderBreakdown.begin();
		 i != _shaderBreakdown.end();
		 ++i)
	{
		Gtk::TreeModel::Row row = *_listStore->append();

		row[_columns.shader] = i->first;
		row[_columns.faceCount] = static_cast<int>(i->second.faceCount);
		row[_columns.patchCount] = static_cast<int>(i->second.patchCount);
	}

	// The table containing the statistics
	Gtk::Table* table = Gtk::manage(new Gtk::Table(1, 2, false));
	_widget->pack_start(*table, false, false, 0);

	Gtk::Label* shaderLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_("Shaders used:")));

	shaderLabel->set_size_request(100, -1);

	table->attach(*shaderLabel, 0, 1, 0, 1,
				  Gtk::AttachOptions(0), Gtk::AttachOptions(0), 0, 0);

	std::string sc = "<b>" + string::to_string(_shaderBreakdown.getMap().size()) + "</b>";

	_shaderCount->set_markup(sc);

	table->attach(*_shaderCount, 1, 2, 0, 1);
}

void ShaderInfoTab::_onSelectItems(bool select)
{
	Gtk::TreeModel::iterator iter = _treeView->get_selection()->get_selected();

	Glib::ustring shader = (*iter)[_columns.shader];

	if (select)
	{
		selection::algorithm::selectItemsByShader(shader);
	}
	else
	{
		selection::algorithm::deselectItemsByShader(shader);
	}
}

bool ShaderInfoTab::_testSelectItems()
{
	// Return positive if there is a selection
	return _treeView->get_selection()->get_selected();
}


} // namespace ui
