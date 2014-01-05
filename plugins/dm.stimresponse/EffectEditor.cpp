#include "EffectEditor.h"

#include "iscenegraph.h"
#include "iregistry.h"
#include "entitylib.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/TreeModel.h"
#include "ResponseEditor.h"
#include <iostream>
#include "i18n.h"
#include "gamelib.h"

#include <gtkmm/table.h>
#include <gtkmm/alignment.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/combobox.h>

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Edit Response Effect");
	const int WINDOW_MIN_WIDTH = 300;
	const int WINDOW_MIN_HEIGHT = 50;

	// The name of the _SELF entity as parsed by the response scripts
	const char* const GKEY_ENTITY_SELF = "/stimResponseSystem/selfEntity";
}

EffectEditor::EffectEditor(const Glib::RefPtr<Gtk::Window>& parent,
						   StimResponse& response,
						   const unsigned int effectIndex,
						   StimTypes& stimTypes,
						   ui::ResponseEditor& editor) :
	gtkutil::BlockingTransientWindow(_(WINDOW_TITLE), parent),
	_argTable(NULL),
	_effectStore(Gtk::ListStore::create(_effectColumns)),
	_entityStore(Gtk::ListStore::create(_entityColumns)),
	_response(response),
	_effectIndex(effectIndex),
	_backup(_response.getResponseEffect(_effectIndex)),
	_editor(editor),
	_stimTypes(stimTypes)
{
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	// Retrieve the map from the ResponseEffectTypes object
	ResponseEffectTypeMap& effectTypes =
		ResponseEffectTypes::Instance().getMap();

	// Now populate the list store with all the possible effect types
	for (ResponseEffectTypeMap::iterator i = effectTypes.begin();
		  i != effectTypes.end(); ++i)
	{
		Gtk::TreeModel::Row row = *_effectStore->append();

		row[_effectColumns.name] = i->first;
		row[_effectColumns.caption] = i->second->getAttribute("editor_caption").getValue();
	}

	// Create the widgets
	populateWindow();

	// Search the scenegraph for entities
	populateEntityListStore();

	// Initialise the widgets with the current data
	ResponseEffect& effect = _response.getResponseEffect(_effectIndex);

	// Setup the selectionfinder to search for the name string
	gtkutil::TreeModel::SelectionFinder finder(effect.getName(), _effectColumns.name.index());

	_effectStore->foreach_iter(
		sigc::mem_fun(finder, &gtkutil::TreeModel::SelectionFinder::forEach));

	// Set the active row of the combo box to the current response effect type
	_effectTypeCombo->set_active(finder.getIter());

	_stateToggle->set_active(effect.isActive());

	// Create the alignment container that hold the (exchangable) widget table
	_argAlignment = Gtk::manage(new Gtk::Alignment(0.0f, 0.5f, 1.0f, 1.0f));
	_argAlignment->set_padding(0, 0, 18, 0);

	_dialogVBox->pack_start(*_argAlignment, false, false, 3);

	// Parse the argument types from the effect and create the widgets
	createArgumentWidgets(effect);

	// Connect the signal to get notified of further changes
	_effectTypeCombo->signal_changed().connect(sigc::mem_fun(*this, &EffectEditor::onEffectTypeChange));

	show();
}

void EffectEditor::populateWindow()
{
	// Create the overall vbox
	_dialogVBox = Gtk::manage(new Gtk::VBox(false, 3));
	add(*_dialogVBox);

	Gtk::HBox* effectHBox = Gtk::manage(new Gtk::HBox(false, 0));

	_effectTypeCombo = Gtk::manage(new Gtk::ComboBox(static_cast<const Glib::RefPtr<Gtk::TreeModel>&>(_effectStore)));

	// Add the cellrenderer for the caption
	Gtk::CellRendererText* captionRenderer = Gtk::manage(new Gtk::CellRendererText);
	_effectTypeCombo->pack_start(*captionRenderer, false);
	_effectTypeCombo->add_attribute(captionRenderer->property_text(), _effectColumns.caption);

	Gtk::Label* effectLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_("Effect:")));

	effectHBox->pack_start(*effectLabel, false, false, 0);
	effectHBox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*_effectTypeCombo, 12, 1.0f)),
		true, true, 0
	);

	_dialogVBox->pack_start(*effectHBox, false, false, 3);

	_stateToggle = Gtk::manage(new Gtk::CheckButton(_("Active")));
	_stateToggle->signal_toggled().connect(sigc::mem_fun(*this, &EffectEditor::onStateToggle));

	_dialogVBox->pack_start(*_stateToggle, false, false, 3);

	Gtk::Label* argLabel =
		Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Arguments") + "</b>"));
	_dialogVBox->pack_start(*argLabel, false, false, 0);

	Gtk::Button* saveButton = Gtk::manage(new Gtk::Button(Gtk::Stock::APPLY));
	saveButton->signal_clicked().connect(sigc::mem_fun(*this, &EffectEditor::onSave));

	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));
	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &EffectEditor::onCancel));

	Gtk::HBox* buttonHBox = Gtk::manage(new Gtk::HBox(false, 0));
	buttonHBox->pack_end(*saveButton, false, false, 0);
	buttonHBox->pack_end(*cancelButton, false, false, 6);

	_dialogVBox->pack_end(*buttonHBox, false, false, 3);
}

void EffectEditor::effectTypeChanged()
{
	std::string newEffectName("");

	// Get the currently selected effect name from the combo box
	Gtk::TreeModel::iterator iter = _effectTypeCombo->get_active();

	if (iter)
	{
		newEffectName = Glib::ustring((*iter)[_effectColumns.name]);
	}

	// Get the effect
	ResponseEffect& effect = _response.getResponseEffect(_effectIndex);

	// Set the name of the effect and trigger an argument list update
	effect.setName(newEffectName);
	effect.clearArgumentList();
	effect.buildArgumentList();

	// Rebuild the argument list basing on this new effect
	createArgumentWidgets(effect);
}

void EffectEditor::createArgumentWidgets(ResponseEffect& effect)
{
	ResponseEffect::ArgumentList& list = effect.getArguments();

	// Remove all possible previous items from the list
	_argumentItems.clear();

	// Remove the old table if there exists one
	if (_argTable != NULL)
	{
		// This removes the old table from the alignment container
		_argAlignment->remove();

		/*// greebo: Increase the refCount of the table to prevent destruction.
		// Destruction would cause weird shutdown crashes.
		// greebo: Is this true for gtkmm, I wonder?
		g_object_ref(G_OBJECT(_argTable));
		gtk_container_remove(GTK_CONTAINER(_argAlignment), _argTable);*/
	}

	// Setup the table with default spacings
	_argTable = Gtk::manage(new Gtk::Table(static_cast<guint>(list.size()), 3, false));
    _argTable->set_col_spacings(12);
    _argTable->set_row_spacings(6);

	_argAlignment->add(*_argTable);

	for (ResponseEffect::ArgumentList::iterator i = list.begin();
		 i != list.end(); ++i)
	{
		int index = i->first;
		ResponseEffect::Argument& arg = i->second;
		ArgumentItemPtr item;

		if (arg.type == "s")
		{
			// Create a new string argument item
			item = ArgumentItemPtr(new StringArgument(arg));
		}
		else if (arg.type == "f") {
			// Create a new string argument item
			item = ArgumentItemPtr(new FloatArgument(arg));
		}
		else if (arg.type == "v") {
			// Create a new vector argument item
			item = ArgumentItemPtr(new VectorArgument(arg));
		}
		else if (arg.type == "e") {
			// Create a new string argument item
			item = ArgumentItemPtr(new EntityArgument(arg, _entityStore));
		}
		else if (arg.type == "b") {
			// Create a new bool item
			item = ArgumentItemPtr(new BooleanArgument(arg));
		}
		else if (arg.type == "t") {
			// Create a new stim type item
			item = ArgumentItemPtr(new StimTypeArgument(arg, _stimTypes));
		}

		if (item != NULL)
		{
			_argumentItems.push_back(item);

			if (arg.type != "b")
			{
				// The label
				_argTable->attach(
					item->getLabelWidget(),
					0, 1, index-1, index, // index starts with 1, hence the -1
					Gtk::FILL, Gtk::AttachOptions(0), 0, 0
				);

				// The edit widgets
				_argTable->attach(
					item->getEditWidget(),
					1, 2, index-1, index // index starts with 1, hence the -1
				);
			}
			else {
				// This is a checkbutton - should be spanned over two columns
				_argTable->attach(
					item->getEditWidget(),
					0, 2, index-1, index, // index starts with 1, hence the -1
					Gtk::FILL, Gtk::AttachOptions(0), 0, 0
				);
			}

			// The help widgets
			_argTable->attach(
				item->getHelpWidget(),
				2, 3, index-1, index, // index starts with 1, hence the -1
				Gtk::AttachOptions(0), Gtk::AttachOptions(0), 0, 0
			);
		}
	}

	// Show the table and all subwidgets
	_argTable->show_all();
}

void EffectEditor::save()
{
	// Request each argument item to save the content into the argument
	for (std::size_t i = 0; i < _argumentItems.size(); ++i)
	{
		_argumentItems[i]->save();
	}

	// Call the update routine of the parent editor
	_editor.update();
}

// Traverse the scenegraph to populate the tree model
void EffectEditor::populateEntityListStore()
{
	std::string selfEntity = game::current::getValue<std::string>(GKEY_ENTITY_SELF);

	// Append the name to the list store
	Gtk::TreeModel::Row row = *_entityStore->append();

	row[_entityColumns.name] = selfEntity;

	// Create a scenegraph walker to traverse the graph
	class EntityFinder :
		public scene::NodeVisitor
	{
		// List store to add to
		const EntityColumns& _columns;
		Glib::RefPtr<Gtk::ListStore> _store;
	public:
		// Constructor
		EntityFinder(const Glib::RefPtr<Gtk::ListStore>& store, const EntityColumns& columns) :
			_columns(columns),
			_store(store)
		{}

		// Visit function
		bool pre(const scene::INodePtr& node)
		{
			Entity* entity = Node_getEntity(node);

			if (entity != NULL)
			{
				// Get the entity name
				std::string entName = entity->getKeyValue("name");

				// Append the name to the list store
				Gtk::TreeModel::Row row = *_store->append();
				row[_columns.name] = entName;

				return false; // don't traverse children if entity found
	        }

	        return true; // traverse children otherwise
		}
	} finder(_entityStore, _entityColumns);

    GlobalSceneGraph().root()->traverse(finder);
}

void EffectEditor::revert()
{
	_response.getResponseEffect(_effectIndex) = _backup;
}

void EffectEditor::onSave()
{
	// Save the arguments into the objects
	save();

	// Call the inherited DialogWindow::destroy method
	destroy();
}

void EffectEditor::onCancel()
{
	revert();

	// Call the inherited DialogWindow::destroy method
	destroy();
}

void EffectEditor::onStateToggle()
{
	_response.getResponseEffect(_effectIndex).setActive(_stateToggle->get_active());
}

void EffectEditor::onEffectTypeChange()
{
	effectTypeChanged();
}

} // namespace ui
