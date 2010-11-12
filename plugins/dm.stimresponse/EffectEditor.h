#ifndef EFFECTEDITOR_H_
#define EFFECTEDITOR_H_

#include "gtkutil/window/BlockingTransientWindow.h"

#include "StimResponse.h"
#include "ResponseEffectTypes.h"
#include "EffectArgumentItem.h"
#include <boost/shared_ptr.hpp>

#include <gtkmm/liststore.h>

namespace Gtk
{
	class VBox;
	class Alignment;
	class CheckButton;
	class Table;
}

namespace ui
{

class ResponseEditor;

class EffectEditor :
	public gtkutil::BlockingTransientWindow
{
private:
	// The overall vbox
	Gtk::VBox* _dialogVBox;

	// The container holding the argument widget table
	Gtk::Alignment* _argAlignment;

	Gtk::Table* _argTable;

	// The list containing the possible effect types
	ResponseEffectTypeMap _effectTypes;

	Gtk::ComboBox* _effectTypeCombo;

	struct EffectColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		EffectColumns() { add(name); add(caption); }

		Gtk::TreeModelColumn<Glib::ustring> name;			// Name
		Gtk::TreeModelColumn<Glib::ustring> caption;		// Caption String
	};

	EffectColumns _effectColumns;
	Glib::RefPtr<Gtk::ListStore> _effectStore;

	// The entity list store
	struct EntityColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		EntityColumns() { add(name); }

		Gtk::TreeModelColumn<Glib::ustring> name;			// Name
	};

	EntityColumns _entityColumns;
	Glib::RefPtr<Gtk::ListStore> _entityStore;

	// The list of argument items
	typedef boost::shared_ptr<EffectArgumentItem> ArgumentItemPtr;
	typedef std::vector<ArgumentItemPtr> ArgumentItemList;
	ArgumentItemList _argumentItems;

	Gtk::CheckButton* _stateToggle;

	// The references to the object we're editing here
	StimResponse& _response;
	unsigned int _effectIndex;

	// The saved StimResponse to revert the changes on cancel
	ResponseEffect _backup;

	// For calling update() when finished editing
	ResponseEditor& _editor;

	StimTypes& _stimTypes;

public:
	/** greebo: Constructor, needs information about parent and the edit target.
	 *
	 * @parent: The parent this window is child of.
	 *
	 * @response: The Stim/Response object the effect is associated with
	 * 			  (this should be a response, although stims work as well).
	 *
	 * @effectIndex: The response effect index within the given Response.
	 *
	 * @stimTypes: The StimTypes helper class
	 *
	 * @editor: The ResponseEditor for calling update() on exit.
	 */
	EffectEditor(const Glib::RefPtr<Gtk::Window>& parent,
				 StimResponse& response,
				 const unsigned int effectIndex,
				 StimTypes& stimTypes,
				 ResponseEditor& editor);

	/** greebo: Creates the widgets
	 */
	void populateWindow();

private:
	/** greebo: Reverts the changes and loads the values from the
	 * 			backup effect object into the edited one.
	 */
	void revert();

	/** greebo: Gets called on effect type changes to update the argument
	 * 			widgets accordingly.
	 */
	void effectTypeChanged();

	/** greebo: Populate the entity list store by traversing the
	 * 			scene graph searching for entities. The names of
	 * 			the entities are stored into the member _entityStore
	 */
	void populateEntityListStore();

	/** greebo: Saves the widget contents into the arguments.
	 */
	void save();

	/** greebo: Parses the response effect for necessary arguments
	 * 			and creates the according widgets.
	 */
	void createArgumentWidgets(ResponseEffect& effect);

	void onEffectTypeChange();
	void onStateToggle();
	void onSave();
	void onCancel();

}; // class EffectEditor

} // namespace ui

#endif /*EFFECTEDITOR_H_*/
