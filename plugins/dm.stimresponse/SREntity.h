#pragma once

#include <vector>
#include <list>
#include <string>
#include "wxutil/TreeModel.h"
#include <memory>

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
	public wxutil::TreeModel::ColumnRecord
{
	SRListColumns() :
		index(add(wxutil::TreeModel::Column::Integer)),
		srClass(add(wxutil::TreeModel::Column::Icon)),
		caption(add(wxutil::TreeModel::Column::IconText)),
		inherited(add(wxutil::TreeModel::Column::Boolean))
	{}

	wxutil::TreeModel::Column index;		// S/R index
	wxutil::TreeModel::Column srClass;		// Type icon
	wxutil::TreeModel::Column caption;		// Caption String
	wxutil::TreeModel::Column inherited;	// Inheritance flag
};

/**
 * greebo: This is the representation of an entity holding S/R keys.
 * Use the load() and save() methods to load/save the spawnargs.
 *
 * The getStimStore() and getResponseStore() methods are available
 * to retrieve the TreeModel for association with a TreeView.
 * The liststore is maintained automatically when the set() method is
 * used to manipulate the data. The SREntity class will hold a reference
 * of the liststore, so client code should not DecRef() it.
 */
class SREntity
{
public:
	// These are the possible key names
	typedef std::vector<SRKey> KeyList;

	// These are the int-indexed Stims/Responses belonging to an entity
	typedef std::list<StimResponse> StimsAndResponses;

private:
	// The local lists of S/R and possible keys
	StimsAndResponses _list;
	KeyList _keys;

	// The liststore representation
	wxutil::TreeModel::Ptr _stimStore;
	wxutil::TreeModel::Ptr _responseStore;

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
	 * 			having the given integer <index>.
	 *
	 * @returns: The ref to the StimResponse or an empty StimResponse object,
	 * 			 if the index was not found.
	 */
	StimResponse& get(int index);

	/** greebo: Adds a new StimResponse and returns the index of the new object.
	 * 			The ListStore is NOT updated with this call to allow setting of
	 * 			the properties before refreshing the treeview.
	 */
	int add();

	/** greebo: Adds a new StimResponse and returns the index of the new object.
	 * 			The ListStore is NOT updated with this call to allow setting of
	 * 			the properties before refreshing the treeview.
	 */
	StimResponse& add(int index);

	/** greebo: Removes the StimResponse object with the given index.
	 * 			This triggers a refresh of the liststores.
	 */
	void remove(int index);

	/** greebo: Duplicates the stim/response with the given index.
	 *
	 * @fromId: The index of the SR to copy from.
	 * @returns: the index of the new duplicate.
	 */
	int duplicate(int fromIndex);

	// Static column definition
	static const SRListColumns& getColumns();

	/**
	 * greebo: Returns the list store containing the stim/response data.
	 * Use this to add the data to a treeview or a combobox.
	 * Don't call DecRef() on this model, just associate it to a TreeView.
	 */
	wxutil::TreeModel::Ptr getStimStore();
	wxutil::TreeModel::Ptr getResponseStore();

	/** greebo: Sets the <key> of the SR with the given <index> to <value>
	 */
	void setProperty(int index, const std::string& key, const std::string& value);

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
	 * StimResponse with the given <index>
	 *
	 * @targetStore: The liststore where the iter should be searched
	 */
	wxDataViewItem getIterForIndex(wxutil::TreeModel& targetStore, int index);

	StimsAndResponses::iterator findByIndex(int index);

private:

	/** greebo: Write the values of the passed StimResponse to the
	 * 	TreeModel using the passed Row.
	 * 	The index stays untouched.
	 *
	 * @row: The row where the data should be inserted to
	 * @sr: the StimResponse object containing the source data
	 */
	void writeToListRow(wxutil::TreeModel::Row& row, StimResponse& sr);

	// Returns the highest Stim/Response index number
	int getHighestIndex();

	// Returns the highest number used by inherited S/R, or 0 if no inherited S/R are present
	int getHighestInheritedIndex();
};
typedef std::shared_ptr<SREntity> SREntityPtr;
