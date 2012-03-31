#include "EntityInfoTab.h"

#include "i18n.h"
#include "iradiant.h"
#include "icounter.h"

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
		const char* const TAB_NAME = N_("Entities");
		const std::string TAB_ICON("cmenu_add_entity.png");
	}

EntityInfoTab::EntityInfoTab() :
	_widget(NULL)
{
	// Create all the widgets
	populateTab();
}

Gtk::Widget& EntityInfoTab::getWidget()
{
	return *_widget;
}

std::string EntityInfoTab::getLabel()
{
	return _(TAB_NAME);
}

std::string EntityInfoTab::getIconName()
{
	return TAB_ICON;
}

void EntityInfoTab::populateTab()
{
	_widget = Gtk::manage(new Gtk::VBox(false, 6));

	// Set the outer space of the vbox
	_widget->set_border_width(12);

	// Create the list store that contains the eclass => count map
	_listStore = Gtk::ListStore::create(_columns);

	// Create the treeview and pack two columns into it
	_treeView = Gtk::manage(new Gtk::TreeView(_listStore));
	_treeView->set_headers_clickable(true);

	Gtk::TreeViewColumn* eclassCol = Gtk::manage(new gtkutil::TextColumn(_("Entity Class"), _columns.eclass));
	eclassCol->set_sort_column(_columns.eclass);

	Gtk::TreeViewColumn* countCol = Gtk::manage(new gtkutil::TextColumn(_("Count"), _columns.count));
	countCol->set_sort_column(_columns.count);

	_treeView->append_column(*eclassCol);
	_treeView->append_column(*countCol);

	_widget->pack_start(*Gtk::manage(new gtkutil::ScrolledFrame(*_treeView)), true, true, 0);

    // Populate the liststore with the entity count information
    for (map::EntityBreakdown::Map::const_iterator i = _entityBreakdown.begin();
		 i != _entityBreakdown.end();
		 i++)
	{
		Gtk::TreeModel::Row row = *_listStore->append();

		row[_columns.eclass] = i->first;
		row[_columns.count] = static_cast<int>(i->second);
	}

	// The table containing the primitive statistics
	Gtk::Table* table = Gtk::manage(new Gtk::Table(3, 2, false));
	_widget->pack_start(*table, false, false, 0);

	_brushCount = Gtk::manage(new gtkutil::LeftAlignedLabel(""));
	_patchCount = Gtk::manage(new gtkutil::LeftAlignedLabel(""));
	_entityCount = Gtk::manage(new gtkutil::LeftAlignedLabel(""));

	Gtk::Label* brushLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_("Brushes:")));
	Gtk::Label* patchLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_("Patches:")));
	Gtk::Label* entityLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_("Entities:")));

	brushLabel->set_size_request(75, -1);
	patchLabel->set_size_request(75, -1);
	entityLabel->set_size_request(75, -1);

	table->attach(*brushLabel, 0, 1, 0, 1,
				  Gtk::AttachOptions(0), Gtk::AttachOptions(0), 0, 0);

	table->attach(*patchLabel, 0, 1, 1, 2,
				  Gtk::AttachOptions(0), Gtk::AttachOptions(0), 0, 0);

	table->attach(*entityLabel, 0, 1, 2, 3,
				  Gtk::AttachOptions(0), Gtk::AttachOptions(0), 0, 0);

	std::string bc = "<b>" + string::to_string(GlobalCounters().getCounter(counterBrushes).get()) + "</b>";
	std::string pc = "<b>" + string::to_string(GlobalCounters().getCounter(counterPatches).get()) + "</b>";
	std::string ec = "<b>" + string::to_string(GlobalCounters().getCounter(counterEntities).get()) + "</b>";

	_brushCount->set_markup(bc);
	_patchCount->set_markup(pc);
	_entityCount->set_markup(ec);

	table->attach(*_brushCount, 1, 2, 0, 1);
	table->attach(*_patchCount, 1, 2, 1, 2);
	table->attach(*_entityCount, 1, 2, 2, 3);
}

} // namespace ui
