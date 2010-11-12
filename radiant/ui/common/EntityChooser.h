#ifndef _ENTITY_CHOOSER_H_
#define _ENTITY_CHOOSER_H_

#include "gtkutil/dialog/DialogElements.h"
#include <map>

#include <gtkmm/liststore.h>
#include <gtkmm/treeselection.h>

namespace ui
{

class EntityChooser :
	public gtkutil::DialogElement
{
private:
	struct EntityChooserColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		EntityChooserColumns() { add(name); }

		Gtk::TreeModelColumn<Glib::ustring> name;
	};

	EntityChooserColumns _listColumns;
	Glib::RefPtr<Gtk::ListStore> _entityStore;
	Glib::RefPtr<Gtk::TreeSelection> _selection;

	std::map<int, Gtk::Widget*> _widgets;

	std::string _selectedEntityName;

public:
	EntityChooser();

	std::string getSelectedEntity() const;
	void setSelectedEntity(const std::string& name);

	// Implementation of StringSerialisable
	virtual std::string exportToString() const;
	virtual void importFromString(const std::string& str);

	/**
	 * Static convenience method. Constructs a dialog with an EntityChooser
	 * and returns the selection.
	 *
	 * @preSelectedEntity: The entity name which should be selected by default.
	 * @returns: The name of the entity or an empty string if the user cancelled the dialog.
	 */
	static std::string ChooseEntity(const std::string& preSelectedEntity);

protected:
	void populateEntityList();

	// gtkmm callback
	void onSelectionChanged();
};
typedef boost::shared_ptr<EntityChooser> EntityChooserPtr;

} // namespace ui

#endif /* _ENTITY_CHOOSER_H_ */
