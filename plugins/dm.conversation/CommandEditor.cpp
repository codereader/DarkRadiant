#include "CommandEditor.h"

#include "i18n.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/TreeModel.h"
#include "string/string.h"

#include <boost/format.hpp>
#include "itextstream.h"

#include <gtkmm/table.h>
#include <gtkmm/alignment.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/combobox.h>

#include "ConversationCommandLibrary.h"

namespace ui {

	namespace {
		const char* const WINDOW_TITLE = N_("Edit Command");
	}

CommandEditor::CommandEditor(const Glib::RefPtr<Gtk::Window>& parent, conversation::ConversationCommand& command, conversation::Conversation conv) :
	gtkutil::BlockingTransientWindow(_(WINDOW_TITLE), parent),
	_conversation(conv),
	_command(command), // copy the conversation command to a local object
	_targetCommand(command),
	_result(NUM_RESULTS),
	_actorStore(Gtk::ListStore::create(_actorColumns)),
	_commandStore(Gtk::ListStore::create(_commandColumns)),
	_argTable(NULL),
	_argumentWidget(NULL)
{
	set_border_width(12);

	// Fill the actor store
	for (conversation::Conversation::ActorMap::const_iterator i = _conversation.actors.begin();
		 i != _conversation.actors.end(); ++i)
	{
		std::string actorStr = (boost::format(_("Actor %d (%s)")) % i->first % i->second).str();

		Gtk::TreeModel::Row row = *_actorStore->append();
		row[_actorColumns.actorNumber] = i->first;
		row[_actorColumns.caption] = actorStr;
	}

	// Let the command library fill the command store
	conversation::ConversationCommandLibrary::Instance().populateListStore(_commandStore, _commandColumns);

	// Create all widgets
	populateWindow();

	// Fill the values
	updateWidgets();

	// Show the editor and block
	show();
}

CommandEditor::Result CommandEditor::getResult()
{
	return _result;
}

void CommandEditor::updateWidgets()
{
	// Select the actor passed from the command
	gtkutil::TreeModel::SelectionFinder finder(_command.actor, _actorColumns.actorNumber.index());

	_actorStore->foreach_iter(sigc::mem_fun(finder, &gtkutil::TreeModel::SelectionFinder::forEach));

	const Gtk::TreeModel::iterator iter = finder.getIter();

	// Select the found treeiter, if the name was found in the liststore
	if (iter)
	{
		_actorDropDown->set_active(iter);
	}

	// Select the type passed from the command
	gtkutil::TreeModel::SelectionFinder cmdFinder(_command.type, _commandColumns.cmdNumber.index());

	_commandStore->foreach_iter(sigc::mem_fun(cmdFinder, &gtkutil::TreeModel::SelectionFinder::forEach));

	const Gtk::TreeModel::iterator cmdIter = cmdFinder.getIter();

	// Select the found treeiter, if the name was found in the liststore
	if (cmdIter)
	{
		_commandDropDown->set_active(cmdIter);
	}

	// Populate the correct command argument widgets
	createArgumentWidgets(_command.type);

	// Pre-fill the values
	for (conversation::ConversationCommand::ArgumentMap::const_iterator i = _command.arguments.begin();
		i != _command.arguments.end(); ++i)
	{
		int argIndex = i->first;

		if (argIndex > static_cast<int>(_argumentItems.size()) || argIndex < 0)
		{
			// Invalid command argument index
			rError() << "Invalid command argument index " << argIndex << std::endl;
			continue;
		}

		// Load the value into the argument item
		_argumentItems[argIndex - 1]->setValueFromString(i->second);
	}

	// Update the "wait until finished" flag
	_waitUntilFinished->set_active(_command.waitUntilFinished);

	// Update the sensitivity of the correct flag
	upateWaitUntilFinished(_command.type);
}

void CommandEditor::save()
{
	// Get the active actor item
	Gtk::TreeModel::iterator actor = _actorDropDown->get_active();

	if (actor)
	{
		_command.actor = (*actor)[_actorColumns.actorNumber];
	}

	// Get the active command type selection
	Gtk::TreeModel::iterator cmd = _commandDropDown->get_active();

	if (cmd)
	{
		_command.type = (*cmd)[_commandColumns.cmdNumber];
	}

	// Clear the existing arguments
	_command.arguments.clear();

	int index = 1;

	for (ArgumentItemList::iterator i = _argumentItems.begin();
		 i != _argumentItems.end(); ++i, ++index)
	{
		_command.arguments[index] = (*i)->getValue();
	}

	// Get the value of the "wait until finished" flag
	try
	{
		const conversation::ConversationCommandInfo& cmdInfo =
			conversation::ConversationCommandLibrary::Instance().findCommandInfo(_command.type);

		if (cmdInfo.waitUntilFinishedAllowed)
		{
			// Load the value
			_command.waitUntilFinished = _waitUntilFinished->get_active();
		}
		else {
			// Command doesn't support "wait until finished", set to default == true
			_command.waitUntilFinished = true;
		}
	}
	catch (std::runtime_error&)
	{
		rError() << "Cannot find conversation command info for index " << _command.type << std::endl;
	}

	// Copy the command over the target object
	_targetCommand = _command;
}

void CommandEditor::populateWindow()
{
	// Create the overall vbox
	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 6));

	// Actor
	vbox->pack_start(
		*Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Actor") + "</b>")),
		false, false, 0);

	// Create the actor dropdown box
	_actorDropDown = Gtk::manage(new Gtk::ComboBox(Glib::RefPtr<Gtk::TreeModel>::cast_static(_actorStore)));

	// Add the cellrenderer for the name
	Gtk::CellRendererText* nameRenderer = Gtk::manage(new Gtk::CellRendererText);

	_actorDropDown->pack_start(*nameRenderer, true);
	_actorDropDown->add_attribute(nameRenderer->property_text(), _actorColumns.caption);

	vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*_actorDropDown, 18, 1)), false, false, 0);

	// Command Type
	vbox->pack_start(
		*Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Command") + "</b>")),
		false, false, 0);

	_commandDropDown = Gtk::manage(new Gtk::ComboBox(Glib::RefPtr<Gtk::TreeModel>::cast_static(_commandStore)));

	// Connect the signal to get notified of further changes
	_commandDropDown->signal_changed().connect(sigc::mem_fun(*this, &CommandEditor::onCommandTypeChange));

	// Add the cellrenderer for the name
	Gtk::CellRendererText* cmdNameRenderer = Gtk::manage(new Gtk::CellRendererText);

	_commandDropDown->pack_start(*cmdNameRenderer, true);
	_commandDropDown->add_attribute(cmdNameRenderer->property_text(), _commandColumns.caption);

	vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*_commandDropDown, 18, 1)), false, false, 0);

	// Command Arguments
	vbox->pack_start(
		*Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Command Arguments") + "</b>")),
		false, false, 0);

	// Create the alignment container that hold the (exchangable) widget table
	_argAlignment = Gtk::manage(new Gtk::Alignment(0.0, 0.5, 1.0, 1.0));
	_argAlignment->set_padding(0, 0, 18, 0);

	vbox->pack_start(*_argAlignment, false, false, 3);

	// Wait until finished
	vbox->pack_start(
		*Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Command Properties") + "</b>")),
		false, false, 0);

	_waitUntilFinished = Gtk::manage(new Gtk::CheckButton(_("Wait until finished")));
	vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*_waitUntilFinished, 18, 1)), false, false, 0);

	// Buttons
	vbox->pack_start(createButtonPanel(), false, false, 0);

	add(*vbox);
}

void CommandEditor::commandTypeChanged()
{
	int newCommandTypeID = -1;

	// Get the currently selected effect name from the combo box
	Gtk::TreeModel::iterator iter = _commandDropDown->get_active();

	if (iter)
	{
		newCommandTypeID = (*iter)[_commandColumns.cmdNumber];
	}

	// Create the argument widgets for this new command type
	createArgumentWidgets(newCommandTypeID);

	// Update the sensitivity of the correct flag
	upateWaitUntilFinished(newCommandTypeID);
}

void CommandEditor::upateWaitUntilFinished(int commandTypeID)
{
	// Update the sensitivity of the correct flag
	try
	{
		const conversation::ConversationCommandInfo& cmdInfo =
			conversation::ConversationCommandLibrary::Instance().findCommandInfo(commandTypeID);

		_waitUntilFinished->set_sensitive(cmdInfo.waitUntilFinishedAllowed);
	}
	catch (std::runtime_error&)
	{
		rError() << "Cannot find conversation command info for index " << commandTypeID << std::endl;
	}
}

void CommandEditor::createArgumentWidgets(int commandTypeID)
{
	try
	{
		const conversation::ConversationCommandInfo& cmdInfo =
			conversation::ConversationCommandLibrary::Instance().findCommandInfo(commandTypeID);

		// Remove all possible previous items from the list
		_argumentItems.clear();

		// Remove the old widget if there exists one
		if (_argumentWidget != NULL)
		{
			// This removes the old table from the alignment container
			// greebo: Increase the refCount of the widget to prevent destruction.
			// Destruction would cause weird shutdown crashes.
			_argumentWidget->reference();
			_argAlignment->remove();
		}

		if (cmdInfo.arguments.empty())
		{
			// No arguments, just push an empty label into the alignment
			Gtk::Label* label = Gtk::manage(new gtkutil::LeftAlignedLabel(
				std::string("<i>") + _("None") + "</i>"));

			_argAlignment->add(*label);

			label->show_all();

			// Remember this widget
			_argumentWidget = label;

			return;
		}

		// Setup the table with default spacings
		_argTable = Gtk::manage(new Gtk::Table(static_cast<guint>(cmdInfo.arguments.size()), 3, false));
		_argTable->set_col_spacings(12);
		_argTable->set_row_spacings(6);

		_argAlignment->add(*_argTable);
		_argumentWidget = _argTable;

		typedef conversation::ConversationCommandInfo::ArgumentInfoList::const_iterator ArgumentIter;

		int index = 1;

		for (ArgumentIter i = cmdInfo.arguments.begin();
			 i != cmdInfo.arguments.end(); ++i, ++index)
		{
			const conversation::ArgumentInfo& argInfo = *i;

			CommandArgumentItemPtr item;

			switch (argInfo.type)
			{
			case conversation::ArgumentInfo::ARGTYPE_BOOL:
				// Create a new bool argument item
				item = CommandArgumentItemPtr(new BooleanArgument(argInfo));
				break;
			case conversation::ArgumentInfo::ARGTYPE_INT:
			case conversation::ArgumentInfo::ARGTYPE_FLOAT:
			case conversation::ArgumentInfo::ARGTYPE_STRING:
				// Create a new string argument item
				item = CommandArgumentItemPtr(new StringArgument(argInfo));
				break;
			case conversation::ArgumentInfo::ARGTYPE_VECTOR:
			case conversation::ArgumentInfo::ARGTYPE_SOUNDSHADER:
				// Create a new string argument item
				item = CommandArgumentItemPtr(new StringArgument(argInfo));
				break;
			case conversation::ArgumentInfo::ARGTYPE_ACTOR:
				// Create a new actor argument item
				item = CommandArgumentItemPtr(new ActorArgument(argInfo, _actorStore, _actorColumns));
				break;
			case conversation::ArgumentInfo::ARGTYPE_ENTITY:
				// Create a new string argument item
				item = CommandArgumentItemPtr(new StringArgument(argInfo));
				break;
			default:
				rError() << "Unknown command argument type: " << argInfo.type << std::endl;
				break;
			};

			if (item != NULL)
			{
				_argumentItems.push_back(item);

				if (argInfo.type != conversation::ArgumentInfo::ARGTYPE_BOOL)
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
				else
				{
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
	catch (std::runtime_error&)
	{
		rError() << "Cannot find conversation command info for index " << commandTypeID << std::endl;
	}
}

Gtk::Widget& CommandEditor::createButtonPanel()
{
	Gtk::HBox* buttonHBox = Gtk::manage(new Gtk::HBox(true, 12));

	// Save button
	Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
	okButton->signal_clicked().connect(sigc::mem_fun(*this, &CommandEditor::onSave));
	buttonHBox->pack_end(*okButton, true, true, 0);

	// Cancel Button
	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));
	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &CommandEditor::onCancel));
	buttonHBox->pack_end(*cancelButton, true, true, 0);

	return *Gtk::manage(new gtkutil::RightAlignment(*buttonHBox));
}

void CommandEditor::onSave()
{
	// First, save to the command object
	_result = RESULT_OK;
	save();

	// Then close the window
	destroy();
}

void CommandEditor::onCancel()
{
	// Just close the window without writing the values
	_result = RESULT_CANCEL;
	destroy();
}

void CommandEditor::onCommandTypeChange()
{
	commandTypeChanged();
}

} // namespace ui
