#include "DifficultyEditor.h"

#include "i18n.h"
#include "iuimanager.h"

#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/TreeModel.h"

#include "ClassNameStore.h"

#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/table.h>
#include <gtkmm/paned.h>
#include <gtkmm/comboboxentry.h>
#include <gtkmm/entrycompletion.h>

namespace ui {

	namespace
	{
		const char* const DIFF_ICON = "sr_icon_custom.png";
		const int TREE_VIEW_MIN_WIDTH = 320;
	}

DifficultyEditor::DifficultyEditor(const std::string& label,
								   const difficulty::DifficultySettingsPtr& settings) :
	_settings(settings),
	_updateActive(false)
{
	// The tab label items (icon + label)
	_labelHBox = Gtk::manage(new Gtk::HBox(false, 3));
	_label = Gtk::manage(new Gtk::Label(label));

	_labelHBox->pack_start(
		*Gtk::manage(new Gtk::Image(GlobalUIManager().getLocalPixbufWithMask(DIFF_ICON))),
    	false, false, 3
    );
	_labelHBox->pack_start(*_label, false, false, 3);

	// The actual editor pane
	_editor = Gtk::manage(new Gtk::VBox(false, 12));

	_settings->updateTreeModel();

	populateWindow();
	updateEditorWidgets();
}

Gtk::Widget& DifficultyEditor::getEditor()
{
	return *_editor;
}

// Returns the label for packing into a GtkNotebook tab.
Gtk::Widget& DifficultyEditor::getNotebookLabel()
{
	return *_labelHBox;
}

void DifficultyEditor::setLabel(const std::string& label)
{
	_label->set_markup(label);
}

void DifficultyEditor::populateWindow()
{
	// Pack the treeview and the editor pane into a GtkPaned
	Gtk::HPaned* paned = Gtk::manage(new Gtk::HPaned);
	paned->add1(createTreeView());
	paned->add2(createEditingWidgets());

	// Pack the pane into the topmost editor container
	_editor->pack_start(*paned, true, true, 0);
}

Gtk::Widget& DifficultyEditor::createTreeView()
{
	// First, create the treeview
	_settingsView = Gtk::manage(new Gtk::TreeView(_settings->getTreeStore()));
	_settingsView->set_size_request(TREE_VIEW_MIN_WIDTH, -1);

	// Connect the tree view selection
	Glib::RefPtr<Gtk::TreeSelection> selection = _settingsView->get_selection();
	selection->signal_changed().connect(sigc::mem_fun(*this, &DifficultyEditor::onSettingSelectionChange));

	// Add columns to this view
	Gtk::CellRendererText* textRenderer = Gtk::manage(new Gtk::CellRendererText);

	Gtk::TreeViewColumn* settingCol = Gtk::manage(new Gtk::TreeViewColumn);
	settingCol->pack_start(*textRenderer, false);

    settingCol->set_title(_("Setting"));
	settingCol->set_sizing(Gtk::TREE_VIEW_COLUMN_AUTOSIZE);
    settingCol->set_spacing(3);

	_settingsView->append_column(*settingCol);

	settingCol->add_attribute(textRenderer->property_text(), _settings->getColumns().description);
	settingCol->add_attribute(textRenderer->property_foreground(), _settings->getColumns().colour);
	settingCol->add_attribute(textRenderer->property_strikethrough(), _settings->getColumns().isOverridden);

	Gtk::ScrolledWindow* frame = Gtk::manage(new gtkutil::ScrolledFrame(*_settingsView));

	// Create the action buttons
	Gtk::HBox* buttonHBox = Gtk::manage(new Gtk::HBox(false, 6));

	// Create button
	_createSettingButton = Gtk::manage(new Gtk::Button(Gtk::Stock::ADD));
	_createSettingButton->signal_clicked().connect(sigc::mem_fun(*this, &DifficultyEditor::onSettingCreate));

	// Delete button
	_deleteSettingButton = Gtk::manage(new Gtk::Button(Gtk::Stock::DELETE));
	_deleteSettingButton->signal_clicked().connect(sigc::mem_fun(*this, &DifficultyEditor::onSettingDelete));

	_refreshButton = Gtk::manage(new Gtk::Button(Gtk::Stock::REFRESH));
	_refreshButton->signal_clicked().connect(sigc::mem_fun(*this, &DifficultyEditor::onRefresh));

	buttonHBox->pack_start(*_createSettingButton, true, true, 0);
	buttonHBox->pack_start(*_deleteSettingButton, true, true, 0);
	buttonHBox->pack_start(*_refreshButton, true, true, 0);

	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 6));
	vbox->pack_start(*frame, true, true, 0);
	vbox->pack_start(*buttonHBox, false, false, 0);

	vbox->set_border_width(12);

	return *vbox;
}

Gtk::Widget& DifficultyEditor::createEditingWidgets()
{
	_editorPane = Gtk::manage(new Gtk::VBox(false, 6));
	_editorPane->set_border_width(12);

	// The "Settings" label
	Gtk::Label* settingsLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Setting") + "</b>"));
	_editorPane->pack_start(*settingsLabel, false, false, 0);

	// The table aligning the editing widgets
	Gtk::Table* table = Gtk::manage(new Gtk::Table(3, 2, false));
    table->set_col_spacings(12);
    table->set_row_spacings(6);

	_editorPane->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*table, 18, 1.0)), false, false, 0);

	// ===== CLASSNAME ======

	Gtk::Label* classNameLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_("Classname:")));

	// Add classname widget
	_classCombo = Gtk::manage(new Gtk::ComboBoxEntry(
		ClassNameStore::Instance().getModel(),
		ClassNameStore::Instance().getColumns().classname
	));

	// Add completion functionality to the combobox entry
	Glib::RefPtr<Gtk::EntryCompletion> completion = Gtk::EntryCompletion::create();
	completion->set_model(ClassNameStore::Instance().getModel());
	completion->set_text_column(ClassNameStore::Instance().getColumns().classname);

	_classCombo->get_entry()->set_completion(completion);

	table->attach(*classNameLabel, 0, 1, 0, 1, Gtk::FILL, Gtk::AttachOptions(0), 0, 0);
	table->attach(*_classCombo, 1, 2, 0, 1);

	// ===== SPAWNARG ======
	_spawnArgEntry = Gtk::manage(new Gtk::Entry);
	Gtk::Label* spawnArgLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_("Spawnarg:")));

	table->attach(*spawnArgLabel, 0, 1, 1, 2, Gtk::FILL, Gtk::AttachOptions(0), 0, 0);
	table->attach(*_spawnArgEntry, 1, 2, 1, 2);

	// ===== ARGUMENT ======
	_argumentEntry = Gtk::manage(new Gtk::Entry);
	Gtk::Label* argumentLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_("Argument:")));

	// The appType chooser
	_appTypeCombo = Gtk::manage(new Gtk::ComboBox(difficulty::Setting::getAppTypeStore()));
	_appTypeCombo->signal_changed().connect(sigc::mem_fun(*this, &DifficultyEditor::onAppTypeChange));

	// Add the cellrenderer for the apptype text
	Gtk::CellRendererText* appTypeRenderer = Gtk::manage(new Gtk::CellRendererText);

	_appTypeCombo->pack_start(*appTypeRenderer, false);
	_appTypeCombo->add_attribute(appTypeRenderer->property_text(), difficulty::Setting::getTreeModelColumns().name);

	// Pack the argument entry and the appType dropdown field together
	Gtk::HBox* argHBox = Gtk::manage(new Gtk::HBox(false, 6));
	argHBox->pack_start(*_argumentEntry, true, true, 0);
	argHBox->pack_start(*_appTypeCombo, false, false, 0);

	table->attach(*argumentLabel, 0, 1, 2, 3, Gtk::FILL, Gtk::AttachOptions(0), 0, 0);
	table->attach(*argHBox, 1, 2, 2, 3);

	// Save button
	Gtk::HBox* buttonHbox = Gtk::manage(new Gtk::HBox(false, 6));

	_saveSettingButton = Gtk::manage(new Gtk::Button(Gtk::Stock::SAVE));
	_saveSettingButton->signal_clicked().connect(sigc::mem_fun(*this, &DifficultyEditor::onSettingSave));

	buttonHbox->pack_start(*_saveSettingButton, false, false, 0);
	_editorPane->pack_start(*Gtk::manage(new gtkutil::RightAlignment(*buttonHbox)), false, false, 0);

	// The "note" text
	_noteText = Gtk::manage(new Gtk::Label);
	_noteText->set_line_wrap(true);
	_editorPane->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*_noteText)), false, false, 6);

	return *_editorPane;
}

int DifficultyEditor::getSelectedSettingId()
{
	Gtk::TreeModel::iterator iter = _settingsView->get_selection()->get_selected();

	return (iter) ? (*iter)[_settings->getColumns().settingId] : -1;
}

void DifficultyEditor::updateEditorWidgets()
{
	_updateActive = true;

	int id = getSelectedSettingId();

	bool editWidgetsSensitive = false;

	std::string noteText;

	if (id != -1)
	{
		// Lookup the setting using className/id combo
		difficulty::SettingPtr setting = _settings->getSettingById(id);

		if (setting != NULL)
		{
			// Activate editing pane
			editWidgetsSensitive = true;

			if (_settings->isOverridden(setting))
			{
				editWidgetsSensitive = false;
				noteText += _("This default setting is overridden, cannot edit.");
			}

			_spawnArgEntry->set_text(setting->spawnArg);
			_argumentEntry->set_text(setting->argument);

			// Now select the eclass passed in the argument
			// Find the entity using a TreeModel traversor
			gtkutil::TreeModel::SelectionFinder finder(
				setting->className, ClassNameStore::Instance().getColumns().classname.index()
			);

			ClassNameStore::Instance().getModel()->foreach_iter(
				sigc::mem_fun(finder, &gtkutil::TreeModel::SelectionFinder::forEach)
			);

			// Select the found treeiter, if the name was found in the liststore
			if (finder.getIter())
			{
				_classCombo->set_active(finder.getIter());
			}

			// Select the appType in the dropdown combo box (search the second column)
			gtkutil::TreeModel::SelectionFinder appTypeFinder(
				setting->appType,
				difficulty::Setting::getTreeModelColumns().type.index()
			);

			_appTypeCombo->get_model()->foreach_iter(
				sigc::mem_fun(appTypeFinder, &gtkutil::TreeModel::SelectionFinder::forEach)
			);

			// Select the found treeiter, if the name was found in the liststore
			if (appTypeFinder.getIter())
			{
				_appTypeCombo->set_active(appTypeFinder.getIter());
			}

			// Set the sensitivity of the argument entry box
			_argumentEntry->set_sensitive(
				(setting->appType == difficulty::Setting::EIgnore) ? false : true
			);

			// We have a treeview selection, lock the classname
			_classCombo->set_sensitive(false);

			// Disable the deletion of default settings
			_deleteSettingButton->set_sensitive((setting->isDefault) ? false : true);
			_saveSettingButton->set_sensitive(true);
		}
	}
	else
	{
		// Nothing selected, disable deletion
		_deleteSettingButton->set_sensitive(false);
		_saveSettingButton->set_sensitive(false);
	}

	// Set editing pane sensitivity
	_editorPane->set_sensitive(editWidgetsSensitive);

	// Set the note text in any case
	_noteText->set_markup(noteText);

	_updateActive = false;
}

void DifficultyEditor::createSetting()
{
	// Unselect everything
	_settingsView->get_selection()->unselect_all();

	// Unlock editing widgets
	_editorPane->set_sensitive(true);

	// Unlock class combo
	_classCombo->set_sensitive(true);
	_saveSettingButton->set_sensitive(true);

	_spawnArgEntry->set_text("");
	_argumentEntry->set_text("");
}

void DifficultyEditor::saveSetting()
{
	// Get the ID of the currently selected item (might be -1 if no selection)
	int id = getSelectedSettingId();

	// Instantiate a new setting and fill the data in
	difficulty::SettingPtr setting(new difficulty::Setting);

	// Load the widget contents
	setting->className = _classCombo->get_active_text();
	setting->spawnArg = _spawnArgEntry->get_text();
	setting->argument = _argumentEntry->get_text();

	// Get the apptype from the dropdown list
	setting->appType = difficulty::Setting::EAssign;

	Gtk::TreeModel::iterator iter = _appTypeCombo->get_active();

	if (iter)
	{
		setting->appType = static_cast<difficulty::Setting::EApplicationType>(
			static_cast<int>((*iter)[difficulty::Setting::getTreeModelColumns().type])
		);
	}

	// Pass the data to the DifficultySettings class to handle it
	id = _settings->save(id, setting);

	// Update the treemodel
	_settings->updateTreeModel();

	// Highlight the setting, it might have been newly created
	selectSettingById(id);
}

void DifficultyEditor::deleteSetting()
{
	// Get the ID of the currently selected item (might be -1 if no selection)
	int id = getSelectedSettingId();

	// Instantiate a new setting and fill the data in
	difficulty::SettingPtr setting = _settings->getSettingById(id);

	if (setting == NULL || setting->isDefault) {
		// Don't delete NULL or default settings
		return;
	}

	// Remove the setting
	_settings->deleteSetting(id);
}

void DifficultyEditor::selectSettingById(int id)
{
	gtkutil::TreeModel::findAndSelectInteger(_settingsView, id, _settings->getColumns().settingId);
}

void DifficultyEditor::onSettingSelectionChange()
{
	// Update editor widgets
	updateEditorWidgets();
}

void DifficultyEditor::onSettingSave()
{
	saveSetting();
}

void DifficultyEditor::onSettingDelete()
{
	deleteSetting();
}

void DifficultyEditor::onSettingCreate()
{
	createSetting();
}

void DifficultyEditor::onRefresh()
{
	_settings->refreshTreeModel();
}

void DifficultyEditor::onAppTypeChange()
{
	if (_updateActive) return;

	// Update the sensitivity of the argument entry widget
	Gtk::TreeModel::iterator iter = _appTypeCombo->get_active();

	if (iter)
	{
		typedef difficulty::Setting::EApplicationType AppType; // shortcut

		AppType appType = static_cast<AppType>(
			static_cast<int>((*iter)[difficulty::Setting::getTreeModelColumns().type])
		);

		_argumentEntry->set_sensitive(
			(appType == difficulty::Setting::EIgnore) ? false : true
		);
	}
}

} // namespace ui
