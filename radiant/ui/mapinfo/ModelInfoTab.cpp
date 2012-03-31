#include "ModelInfoTab.h"

#include "i18n.h"
#include "string/convert.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/LeftAlignedLabel.h"

#include <gtkmm/treeview.h>
#include <gtkmm/box.h>
#include <gtkmm/table.h>

namespace ui
{
	namespace
	{
		const char* const TAB_NAME = N_("Models");
		const std::string TAB_ICON("model16green.png");
	}

ModelInfoTab::ModelInfoTab() :
	_widget(NULL)
{
	// Create all the widgets
	populateTab();
}

Gtk::Widget& ModelInfoTab::getWidget()
{
	return *_widget;
}

std::string ModelInfoTab::getLabel() {
	return _(TAB_NAME);
}

std::string ModelInfoTab::getIconName() {
	return TAB_ICON;
}

void ModelInfoTab::populateTab()
{
	_widget = Gtk::manage(new Gtk::VBox(false, 6));

	// Set the outer space of the vbox
	_widget->set_border_width(12);

	// Create the list store that contains the eclass => count map
	_listStore = Gtk::ListStore::create(_columns);

	// Create the treeview and pack two columns into it
	_treeView = Gtk::manage(new Gtk::TreeView(_listStore));
	_treeView->set_headers_clickable(true);

	Gtk::TreeViewColumn* modelCol = Gtk::manage(new gtkutil::TextColumn(_("Model"), _columns.model));
	modelCol->set_sort_column(_columns.model);

	Gtk::TreeViewColumn* polyCountCol = Gtk::manage(new gtkutil::TextColumn(_("Polys"), _columns.polycount));
	polyCountCol->set_sort_column(_columns.polycount);

	Gtk::TreeViewColumn* modelCountCol = Gtk::manage(new gtkutil::TextColumn(_("Count"), _columns.modelcount));
	modelCountCol->set_sort_column(_columns.modelcount);

	Gtk::TreeViewColumn* skinCountCol = Gtk::manage(new gtkutil::TextColumn(_("Skins"), _columns.skincount));
	skinCountCol->set_sort_column(_columns.skincount);

	_treeView->append_column(*modelCol);
	_treeView->append_column(*polyCountCol);
	_treeView->append_column(*modelCountCol);
	_treeView->append_column(*skinCountCol);

    _widget->pack_start(*Gtk::manage(new gtkutil::ScrolledFrame(*_treeView)), true, true, 0);

    // Populate the liststore with the entity count information
    for (map::ModelBreakdown::Map::const_iterator i = _modelBreakdown.begin();
		 i != _modelBreakdown.end();
		 ++i)
	{
		Gtk::TreeModel::Row row = *_listStore->append();

		row[_columns.model] = i->first;
		row[_columns.polycount] = static_cast<int>(i->second.polyCount);
		row[_columns.modelcount] = static_cast<int>(i->second.count);
		row[_columns.skincount] = static_cast<int>(i->second.skinCount.size());
	}

	// The table containing the statistics
	Gtk::Table* table = Gtk::manage(new Gtk::Table(2, 2, false));
	_widget->pack_start(*table, false, false, 0);

	_modelCount = Gtk::manage(new gtkutil::LeftAlignedLabel(""));
	_skinCount = Gtk::manage(new gtkutil::LeftAlignedLabel(""));

	Gtk::Label* modelsLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_("Models used:")));
	Gtk::Label* skinsLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_("Named Skins used:")));

	modelsLabel->set_size_request(120, -1);
	skinsLabel->set_size_request(120, -1);

	table->attach(*modelsLabel, 0, 1, 0, 1,
				  Gtk::AttachOptions(0), Gtk::AttachOptions(0), 0, 0);

	table->attach(*skinsLabel, 0, 1, 1, 2,
				  Gtk::AttachOptions(0), Gtk::AttachOptions(0), 0, 0);

	std::string mc = "<b>" + string::to_string(_modelBreakdown.getMap().size()) + "</b>";
	std::string sc = "<b>" + string::to_string(_modelBreakdown.getNumSkins()) + "</b>";

	_modelCount->set_markup(mc);
	_skinCount->set_markup(sc);

	table->attach(*_modelCount, 1, 2, 0, 1);
	table->attach(*_skinCount, 1, 2, 1, 2);
}

} // namespace ui
