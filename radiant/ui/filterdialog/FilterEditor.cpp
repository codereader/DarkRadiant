#include "FilterEditor.h"

#include "i18n.h"

#include "gtkutil/RightAlignment.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/ScrolledFrame.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/entry.h>
#include <gtkmm/stock.h>
#include <gtkmm/cellrenderercombo.h>
#include <gtkmm/treeview.h>

#include "shaderlib.h"

namespace ui
{
	namespace
	{
		const int DEFAULT_SIZE_X = 550;
	    const int DEFAULT_SIZE_Y = 350;
		const char* const WINDOW_TITLE_EDIT = N_("Edit Filter");
		const char* const WINDOW_TITLE_VIEW = N_("View Filter");

		const char* const RULE_HELP_TEXT =
			N_("Filter rules are applied in the shown order.\n" \
			"<b>Match</b> is accepting regular expressions.\n" \
			"<b>Object</b>-type filters can be used to match <b>patch</b> or <b>brush</b>.");

		enum
		{
			WIDGET_NAME_ENTRY,
			WIDGET_ADD_RULE_BUTTON,
			WIDGET_MOVE_RULE_UP_BUTTON,
			WIDGET_MOVE_RULE_DOWN_BUTTON,
			WIDGET_DELETE_RULE_BUTTON,
			WIDGET_HELP_TEXT,
		};
	}

FilterEditor::FilterEditor(Filter& filter, const Glib::RefPtr<Gtk::Window>& parent, bool viewOnly) :
	BlockingTransientWindow(viewOnly ? _(WINDOW_TITLE_VIEW) : _(WINDOW_TITLE_EDIT), parent),
	_originalFilter(filter),
	_filter(_originalFilter), // copy-construct
	_ruleStore(Gtk::ListStore::create(_columns)),
	_ruleView(NULL),
	_typeStore(Gtk::ListStore::create(_typeStoreColumns)),
	_actionStore(Gtk::ListStore::create(_actionStoreColumns)),
	_selectedRule(-1),
	_result(NUM_RESULTS),
	_updateActive(false),
	_viewOnly(viewOnly)
{
	set_default_size(DEFAULT_SIZE_X, DEFAULT_SIZE_Y);
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	// Create the widgets
	populateWindow();

	// Update the widget contents
	update();

	// Show and block
	show();
}

FilterEditor::Result FilterEditor::getResult()
{
	return _result;
}

void FilterEditor::populateWindow()
{
	// Create the dialog vbox
	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 6));

	// Create the name entry box
	vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(
		std::string("<b>") + _("Name") + "</b>")), false, false, 0);

	Gtk::Entry* entry = Gtk::manage(new Gtk::Entry);
	_widgets[WIDGET_NAME_ENTRY] = entry;
	vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*entry, 18, 1)), false, false, 0);

	entry->signal_changed().connect(sigc::mem_fun(*this, &FilterEditor::onNameEdited));

	// And the rule treeview
	vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(
		std::string("<b>") + _("Rules") + "</b>")), false, false, 0);

	vbox->pack_start(createCriteriaPanel(), true, true, 0);

	// Add the help text
	if (!_viewOnly)
	{
		_widgets[WIDGET_HELP_TEXT] = Gtk::manage(new gtkutil::LeftAlignedLabel(_(RULE_HELP_TEXT)));
		vbox->pack_start(*_widgets[WIDGET_HELP_TEXT], false, false, 0);
	}

	// Buttons
	vbox->pack_start(createButtonPanel(), false, false, 0);

	add(*vbox);
}

void FilterEditor::update()
{
	// Avoid callback loops
	_updateActive = true;

	// Populate the criteria store
	_ruleStore->clear();

	_selectedRule = -1;

	// Traverse the criteria of the Filter to be edited
	for (std::size_t i = 0; i < _filter.rules.size(); ++i)
	{
		const FilterRule& rule = _filter.rules[i];

		// Allocate a new list store element and store its pointer into <iter>
		Gtk::TreeModel::Row row = *_ruleStore->append();

		row[_columns.index] = static_cast<int>(i);
		row[_columns.type] = static_cast<int>(rule.type);
		row[_columns.typeString] = getStringForType(rule.type);
		row[_columns.regexMatch] = rule.match;
		row[_columns.entityKey] = rule.entityKey;
		row[_columns.showHide] = rule.show ? std::string(_("show")) : std::string(_("hide"));
	}

	static_cast<Gtk::Entry*>(_widgets[WIDGET_NAME_ENTRY])->set_text(_filter.name);

	updateWidgetSensitivity();

	_updateActive = false;
}

Gtk::Widget& FilterEditor::createCriteriaPanel()
{
	// Create an hbox for the treeview and the action buttons
	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 6));

	// Create a new treeview
	_ruleView = Gtk::manage(new Gtk::TreeView(_ruleStore));

	gtkutil::TextColumn* indexCol = Gtk::manage(new gtkutil::TextColumn(_("Index"), _columns.index));
	gtkutil::TextColumn* regexCol = Gtk::manage(new gtkutil::TextColumn(_("Match"), _columns.regexMatch));
	gtkutil::TextColumn* entityKeyCol = Gtk::manage(new gtkutil::TextColumn(_("Entity Key"), _columns.entityKey));

	// Create the cell renderer for the action choice
	Gtk::CellRendererCombo* actionComboRenderer = Gtk::manage(new Gtk::CellRendererCombo);

	actionComboRenderer->property_has_entry() = false;
	actionComboRenderer->property_text_column() = _actionStoreColumns.action.index();
	actionComboRenderer->property_editable() = true;
	actionComboRenderer->property_model() = createActionStore();

	// Construct the column itself
	Gtk::TreeViewColumn* actionCol = Gtk::manage(new Gtk::TreeViewColumn(_("Action"), *actionComboRenderer));
	actionCol->add_attribute(actionComboRenderer->property_markup(), _columns.showHide);

	actionComboRenderer->signal_edited().connect(sigc::mem_fun(*this, &FilterEditor::onActionEdited));

	// Regex editing
	Gtk::CellRendererText* rend = regexCol->getCellRenderer();
	rend->property_editable() = true;
	rend->signal_edited().connect(sigc::mem_fun(*this, &FilterEditor::onRegexEdited));

	// Entity Key editing
	rend = entityKeyCol->getCellRenderer();
	rend->property_editable() = true;
	rend->signal_edited().connect(sigc::mem_fun(*this, &FilterEditor::onEntityKeyEdited));

	// Create the cell renderer for the type choice
	Gtk::CellRendererCombo* typeComboRenderer = Gtk::manage(new Gtk::CellRendererCombo);

	typeComboRenderer->property_has_entry() = false;
	typeComboRenderer->property_text_column() = _typeStoreColumns.typeString.index();
	typeComboRenderer->property_editable() = true;
	typeComboRenderer->property_model() = createTypeStore();

	// Construct the column itself
	Gtk::TreeViewColumn* typeCol = Gtk::manage(new Gtk::TreeViewColumn(_("Type"), *typeComboRenderer));
	typeCol->add_attribute(typeComboRenderer->property_markup(), _columns.typeString);

	typeComboRenderer->signal_edited().connect(sigc::mem_fun(*this, &FilterEditor::onTypeEdited));

	_ruleView->append_column(*indexCol);
	_ruleView->append_column(*typeCol);
	_ruleView->append_column(*entityKeyCol);
	_ruleView->append_column(*regexCol);
	_ruleView->append_column(*actionCol);

	Glib::RefPtr<Gtk::TreeSelection> sel = _ruleView->get_selection();
	sel->signal_changed().connect(sigc::mem_fun(*this, &FilterEditor::onRuleSelectionChanged));

	// Action buttons
	Gtk::Button* addRuleButton = Gtk::manage(new Gtk::Button(Gtk::Stock::ADD));
	Gtk::Button* moveRuleUpButton = Gtk::manage(new Gtk::Button(Gtk::Stock::GO_UP));
	Gtk::Button* moveRuleDownButton = Gtk::manage(new Gtk::Button(Gtk::Stock::GO_DOWN));
	Gtk::Button* deleteRuleButton = Gtk::manage(new Gtk::Button(Gtk::Stock::DELETE));

	_widgets[WIDGET_ADD_RULE_BUTTON] = addRuleButton;
	_widgets[WIDGET_MOVE_RULE_UP_BUTTON] = moveRuleUpButton;
	_widgets[WIDGET_MOVE_RULE_DOWN_BUTTON] = moveRuleDownButton;
	_widgets[WIDGET_DELETE_RULE_BUTTON] = deleteRuleButton;

	addRuleButton->signal_clicked().connect(sigc::mem_fun(*this, &FilterEditor::onAddRule));
	moveRuleUpButton->signal_clicked().connect(sigc::mem_fun(*this, &FilterEditor::onMoveRuleUp));
	moveRuleDownButton->signal_clicked().connect(sigc::mem_fun(*this, &FilterEditor::onMoveRuleDown));
	deleteRuleButton->signal_clicked().connect(sigc::mem_fun(*this, &FilterEditor::onDeleteRule));

	Gtk::VBox* actionVBox = Gtk::manage(new Gtk::VBox(false, 6));

	actionVBox->pack_start(*_widgets[WIDGET_ADD_RULE_BUTTON], false, false, 0);
	actionVBox->pack_start(*_widgets[WIDGET_MOVE_RULE_UP_BUTTON], false, false, 0);
	actionVBox->pack_start(*_widgets[WIDGET_MOVE_RULE_DOWN_BUTTON], false, false, 0);
	actionVBox->pack_start(*_widgets[WIDGET_DELETE_RULE_BUTTON], false, false, 0);

	hbox->pack_start(*Gtk::manage(new gtkutil::ScrolledFrame(*_ruleView)), true, true, 0);
	hbox->pack_start(*actionVBox, false, false, 0);

	return *Gtk::manage(new gtkutil::LeftAlignment(*hbox, 18, 1));
}

const Glib::RefPtr<Gtk::ListStore>& FilterEditor::createTypeStore()
{
	// Create the typestore
	_typeStore = Gtk::ListStore::create(_typeStoreColumns);

	int index = 0;

	Gtk::TreeModel::Row row = *_typeStore->append();
	row[_typeStoreColumns.type] = index++;
	row[_typeStoreColumns.typeString] = Glib::ustring("entityclass");

	row = *_typeStore->append();
	row[_typeStoreColumns.type] = index++;
	row[_typeStoreColumns.typeString] = Glib::ustring("texture");

	row = *_typeStore->append();
	row[_typeStoreColumns.type] = index++;
	row[_typeStoreColumns.typeString] = Glib::ustring("object");

	row = *_typeStore->append();
	row[_typeStoreColumns.type] = index++;
	row[_typeStoreColumns.typeString] = Glib::ustring("entitykeyvalue");

	return _typeStore;
}

const Glib::RefPtr<Gtk::ListStore>& FilterEditor::createActionStore()
{
	// Create the typestore
	_actionStore = Gtk::ListStore::create(_actionStoreColumns);

	Gtk::TreeModel::Row row = *_actionStore->append();
	row[_actionStoreColumns.boolean] = true;
	row[_actionStoreColumns.action] = Glib::ustring(_("show"));

	row = *_actionStore->append();
	row[_actionStoreColumns.boolean] = false;
	row[_actionStoreColumns.action] = Glib::ustring(_("hide"));

	return _actionStore;
}

Gtk::Widget& FilterEditor::createButtonPanel()
{
	Gtk::HBox* buttonHBox = Gtk::manage(new Gtk::HBox(true, 12));

	if (_viewOnly)
	{
		// OK button
		Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
		// Connect the OK button to the "CANCEL" event
		okButton->signal_clicked().connect(sigc::mem_fun(*this, &FilterEditor::onCancel));
		buttonHBox->pack_end(*okButton, true, true, 0);
	}
	else
	{
		// Save button
		Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
		okButton->signal_clicked().connect(sigc::mem_fun(*this, &FilterEditor::onSave));

		buttonHBox->pack_end(*okButton, true, true, 0);

		// Cancel Button
		Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));
		cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &FilterEditor::onCancel));

		buttonHBox->pack_end(*cancelButton, true, true, 0);
	}

	return *Gtk::manage(new gtkutil::RightAlignment(*buttonHBox));
}

std::string FilterEditor::getStringForType(const FilterRule::Type type)
{
	switch (type)
	{
	case FilterRule::TYPE_TEXTURE: return "texture";
	case FilterRule::TYPE_OBJECT: return "object";
	case FilterRule::TYPE_ENTITYCLASS: return "entityclass";
	case FilterRule::TYPE_ENTITYKEYVALUE: return "entitykeyvalue";
	default: return "";
	};
}

FilterRule::Type FilterEditor::getTypeForString(const std::string& typeStr)
{
	if (typeStr == "texture")
	{
		return FilterRule::TYPE_TEXTURE;
	}
	else if (typeStr == "object")
	{
		return FilterRule::TYPE_OBJECT;
	}
	else if (typeStr == "entityclass")
	{
		return FilterRule::TYPE_ENTITYCLASS;
	}
	else // "entitykeyvalue"
	{
		return FilterRule::TYPE_ENTITYKEYVALUE;
	}
}

void FilterEditor::save()
{
	_filter.name = static_cast<Gtk::Entry*>(_widgets[WIDGET_NAME_ENTRY])->get_text();

	// Copy the working set over the actual Filter
	_originalFilter = _filter;
}

void FilterEditor::updateWidgetSensitivity() {

	if (_viewOnly)
	{
		_widgets[WIDGET_NAME_ENTRY]->set_sensitive(false);
		_ruleView->set_sensitive(false);

		_widgets[WIDGET_ADD_RULE_BUTTON]->set_sensitive(false);
		_widgets[WIDGET_MOVE_RULE_UP_BUTTON]->set_sensitive(false);
		_widgets[WIDGET_MOVE_RULE_DOWN_BUTTON]->set_sensitive(false);
		_widgets[WIDGET_DELETE_RULE_BUTTON]->set_sensitive(false);
		return;
	}

	if (_selectedRule != -1)
	{
		bool lastSelected = (_selectedRule + 1 >= static_cast<int>(_filter.rules.size()) || _filter.rules.size() <= 1);
		bool firstSelected = (_selectedRule <= 0 || _filter.rules.size() <= 1);

		_widgets[WIDGET_MOVE_RULE_UP_BUTTON]->set_sensitive(!firstSelected);
		_widgets[WIDGET_MOVE_RULE_DOWN_BUTTON]->set_sensitive(!lastSelected);
		_widgets[WIDGET_DELETE_RULE_BUTTON]->set_sensitive(true);
	}
	else
	{
		// no rule selected
		_widgets[WIDGET_MOVE_RULE_UP_BUTTON]->set_sensitive(false);
		_widgets[WIDGET_MOVE_RULE_DOWN_BUTTON]->set_sensitive(false);
		_widgets[WIDGET_DELETE_RULE_BUTTON]->set_sensitive(false);
	}
}

void FilterEditor::onSave()
{
	save();
	_result = RESULT_OK;
	destroy();
}

void FilterEditor::onCancel()
{
	_result = RESULT_CANCEL;
	destroy();
}

void FilterEditor::onRegexEdited(const Glib::ustring& path, const Glib::ustring& new_text)
{
	Gtk::TreeModel::iterator iter = _ruleStore->get_iter(path);

	if (iter)
	{
		// The iter points to the edited cell now, get the criterion number
		Gtk::TreeModel::Row row = *iter;

		int index = row[_columns.index];

		// Update the criterion
		assert(index >= 0 && index < static_cast<int>(_filter.rules.size()));

		_filter.rules[index].match = new_text;

		// Update the liststore item
		row[_columns.regexMatch] = new_text;
	}
}

void FilterEditor::onEntityKeyEdited(const Glib::ustring& path, const Glib::ustring& new_text)
{
	Gtk::TreeModel::iterator iter = _ruleStore->get_iter(path);

	if (iter)
	{
		// The iter points to the edited cell now, get the criterion number
		Gtk::TreeModel::Row row = *iter;

		int index = row[_columns.index];

		// Update the criterion
		assert(index >= 0 && index < static_cast<int>(_filter.rules.size()));

		_filter.rules[index].entityKey = new_text;

		// Update the liststore item
		row[_columns.entityKey] = new_text;
	}
}

void FilterEditor::onTypeEdited(const Glib::ustring& path, const Glib::ustring& new_text)
{
	Gtk::TreeModel::iterator iter = _ruleStore->get_iter(path);

	if (iter)
	{
		// The iter points to the edited cell
		Gtk::TreeModel::Row row = *iter;

		// Look up the type index for "new_text"
		FilterRule::Type type = getTypeForString(new_text);

		// Get the criterion number
		int index = row[_columns.index];

		// Update the criterion
		assert(index >= 0 && index < static_cast<int>(_filter.rules.size()));

		_filter.rules[index].type = type;

		// Update the liststore item
		row[_columns.type] = type;
		row[_columns.typeString] = new_text;
	}
}

void FilterEditor::onActionEdited(const Glib::ustring& path, const Glib::ustring& new_text)
{
	Gtk::TreeModel::iterator iter = _ruleStore->get_iter(path);

	if (iter)
	{
		// The iter points to the edited cell
		Gtk::TreeModel::Row row = *iter;

		// Get the criterion number
		int index = row[_columns.index];

		// Update the criterion
		assert(index >= 0 && index < static_cast<int>(_filter.rules.size()));

		// Update the bool flag
		_filter.rules[index].show = (new_text == _("show"));

		// Update the liststore item
		row[_columns.showHide] = new_text;
	}
}

void FilterEditor::onNameEdited()
{
	if (_updateActive) return;

	_filter.name = static_cast<Gtk::Entry*>(_widgets[WIDGET_NAME_ENTRY])->get_text();
}

void FilterEditor::onRuleSelectionChanged()
{
	Gtk::TreeModel::iterator iter = _ruleView->get_selection()->get_selected();

	if (iter)
	{
		Gtk::TreeModel::Row row = *iter;
		_selectedRule = row[_columns.index];
	}
	else
	{
		_selectedRule = -1;
	}

	updateWidgetSensitivity();
}

void FilterEditor::onAddRule()
{
	FilterRule newRule = FilterRule::Create(FilterRule::TYPE_TEXTURE, GlobalTexturePrefix_get(), false);
	_filter.rules.push_back(newRule);

	update();
}

void FilterEditor::onMoveRuleUp()
{
	if (_selectedRule >= 1)
	{
		FilterRule temp = _filter.rules[_selectedRule - 1];
		_filter.rules[_selectedRule - 1] = _filter.rules[_selectedRule];
		_filter.rules[_selectedRule] = temp;

		update();
	}
}

void FilterEditor::onMoveRuleDown()
{
	if (_selectedRule < static_cast<int>(_filter.rules.size()) - 1)
	{
		FilterRule temp = _filter.rules[_selectedRule + 1];
		_filter.rules[_selectedRule + 1] = _filter.rules[_selectedRule];
		_filter.rules[_selectedRule] = temp;

		update();
	}
}

void FilterEditor::onDeleteRule()
{
	if (_selectedRule != -1)
	{
		// Let the rules slip down one index each
		for (std::size_t i = _selectedRule; i+1 < _filter.rules.size(); ++i)
		{
			_filter.rules[i] = _filter.rules[i+1];
		}

		// Remove one item, it is the superfluous one now
		_filter.rules.pop_back();

		update();
	}
}

} // namespace ui
