#ifndef SRENTITY_H_
#define SRENTITY_H_

#include <vector>
#include <list>
#include <string>
#include <gtkmm/liststore.h>
#include <boost/shared_ptr.hpp>

#include "StimResponse.h"
#include "StimTypes.h"

// Forward declaration
class Entity;

namespace
{
	const std::string ICON_STIM = "sr_stim";
	const std::string ICON_RESPONSE = "sr_response";
	const std::string ICON_CUSTOM_STIM = "sr_icon_custom.png";

	const std::string SUFFIX_INHERITED = "_inherited";
	const std::string SUFFIX_INACTIVE = "_inactive";
	const std::string SUFFIX_EXTENSION = ".png";
}

// Tree model definition for a Stim/Response list
struct SRListColumns :
	public Gtk::TreeModel::ColumnRecord
{
	SRListColumns()
	{
		add(index);
		add(srClass);
		add(caption);
		add(icon);
		add(inherited);
		add(id);
		add(colour);
	}

	Gtk::TreeModelColumn<int> index;						// S/R index
	Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > srClass; // Type icon
	Gtk::TreeModelColumn<Glib::ustring> caption;			// Caption String
	Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > icon;	// Icon
	Gtk::TreeModelColumn<bool> inherited;					// Inheritance flag
	Gtk::TreeModelColumn<int> id;							// ID (unique)
	Gtk::TreeModelColumn<Glib::ustring> colour;				// Text colour
};

/** greebo: This is the representation of an entity holding S/R keys.
 * 			Use the load() and save() methods to load/save the spawnargs.
 *
 * 			The getStimStore() and getResponseStore() methods are available
 * 			to retrieve the GtkListStores to pack the data into a GtkTreeView.
 * 			The liststore is maintained automatically when the set() method is
 * 			used to manipulate the data.
 */
class SREntity
{
public:
	// These are the possible key names
	typedef std::vector<SRKey> KeyList;

	// These are the int-indexed Stims/Responses belonging to an entity
	typedef std::map<int, StimResponse> StimResponseMap;

private:
	// The local lists of S/R and possible keys
	StimResponseMap _list;
	KeyList _keys;

	// The liststore representation
	Glib::RefPtr<Gtk::ListStore> _stimStore;
	Glib::RefPtr<Gtk::ListStore> _responseStore;

	// A collection of warnings regarding the parsing of the spawnargs
	std::string _warnings;

	// The helper class managing the various stim types
	StimTypes& _stimTypes;

	// An empty stim
	StimResponse _emptyStimResponse;

public:
	SREntity(Entity* source, StimTypes& stimTypes);

	void load(Entity* source);
	void save(Entity* target);

	/** greebo: Removes all the stim/response-relevant spawnargs
	 * 			from the <target> entity.
	 */
	void cleanEntity(Entity* target);

	/** greebo: Retrieves the reference to the StimResponse object
	 * 			having the given integer <id>.
	 *
	 * @returns: The ref to the StimResponse or an empty StimResponse object,
	 * 			 if the id was not found.
	 */
	StimResponse& get(int id);

	/** greebo: Adds a new StimResponse and returns the id of the new object.
	 * 			The ListStore is NOT updated with this call to allow setting of
	 * 			the properties before refreshing the treeview.
	 */
	int add();

	/** greebo: Removes the StimResponse object with the given id.
	 * 			This triggers a refresh of the liststores.
	 */
	void remove(int id);

	/** greebo: Duplicates the stim/response with the given id.
	 *
	 * @fromId: The ID of the SR to copy from.
	 * @returns: the ID of the new duplicate.
	 */
	int duplicate(int fromId);

	/** greebo: Overrides the "state" property of an inherited stim.
	 * 			As inherited spawnargs can't be altered, a sr_state_N
	 * 			key/value pair is added to the entity, overriding
	 * 			the inherited one.
	 */
	void setInheritedState(int id, bool enabled);

	/** greebo: Returns TRUE if the inherited stim/response is enabled,
	 * 			FALSE, if the inherited item is overridden.
	 */
	bool getInheritedState(int id);

	// Static column definition
	static const SRListColumns& getColumns();

	/**
	 * greebo: Returns the list store containing the stim/response data.
	 * Use this to add the data to a treeview or a combobox.
	 */
	const Glib::RefPtr<Gtk::ListStore>& getStimStore();
	const Glib::RefPtr<Gtk::ListStore>& getResponseStore();

	/** greebo: Sets the <key> of the SR with the given <id> to <value>
	 */
	void setProperty(int id, const std::string& key, const std::string& value);

	/** greebo: Updates the ListStore according to the
	 * 			values of the current StimResponseMap <_list>
	 */
	void updateListStores();

	/** greebo: Helper to load the possible key names
	 * 			from the registry into the _keys list.
	 */
	void loadKeys();

	/**
	 * greebo: Returns the treeIter pointing to the row containing the
	 * StimResponse with the given <id>
	 *
	 * @targetStore: The liststore where the iter should be searched
	 */
	Gtk::TreeModel::iterator getIterForId(const Glib::RefPtr<Gtk::ListStore>& targetStore, int id);

private:
	/** greebo: Write the values of the passed StimResponse to the
	 * 			GtkListStore using the passed GtkTreeIter.
	 * 			The ID stays untouched.
	 *
	 * @row: The row where the data should be inserted to
	 * @sr: the StimResponse object containing the source data
	 */
	void writeToListRow(const Gtk::TreeModel::Row& row, StimResponse& sr);

	// Returns the highest currently assigned id
	int getHighestId();

	// Returns the highest Stim/Response index number
	int getHighestIndex();
};

typedef boost::shared_ptr<SREntity> SREntityPtr;

#endif /*SRENTITY_H_*/
