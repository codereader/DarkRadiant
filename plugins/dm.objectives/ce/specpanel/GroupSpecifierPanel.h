#ifndef GROUPSPECIFIERPANEL_H_
#define GROUPSPECIFIERPANEL_H_

#include "TextSpecifierPanel.h"
#include <gtkmm/liststore.h>

namespace objectives
{

namespace ce
{

/**
 * SpecifierPanel subclass for the SPEC_GROUP specifier type.
 * It provides a text entry box with auto-completion functionality
 * for a few special cases like "loot_gold" etc.
 */
class GroupSpecifierPanel :
	public TextSpecifierPanel
{
private:
	// Map registration
	static struct RegHelper {
		RegHelper() {
			SpecifierPanelFactory::registerType(
				SpecifierType::SPEC_GROUP().getName(),
				SpecifierPanelPtr(new GroupSpecifierPanel())
			);
		}
	} _regHelper;

	struct ListColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		ListColumns() { add(name); }

		Gtk::TreeModelColumn<Glib::ustring> name;
	};

	ListColumns _columns;
	Glib::RefPtr<Gtk::ListStore> _listStore;

public:
	/**
	 * Construct a GroupSpecifierPanel.
	 */
	GroupSpecifierPanel();

	// SpecifierPanel implementation
	SpecifierPanelPtr clone() const
	{
		return SpecifierPanelPtr(new GroupSpecifierPanel());
	}

private:
	// Creates and fills the auto-completion liststore for this specifier panel
	void populateCompletionListStore();
};

} // namespace objectives

} // namespace ce

#endif /* GROUPSPECIFIERPANEL_H_ */
