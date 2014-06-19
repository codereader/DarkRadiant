#pragma once

#include <map>
#include <string>
#include "ientity.h"
#include "gtkutil/TreeModel.h"

class wxBitmapComboBox;
class wxComboBox;

/** greebo: A simple StimType representation.
 */
struct StimType {
	std::string name;			// The name (STIM_FIRE)
	std::string caption;		// Nice format ("Fire")
	std::string description;	// Unused at the moment
	std::string icon;			// The icon to display
	bool custom;				// is TRUE for custom stims
};
typedef std::map<int, StimType> StimTypeMap;

class StimTypes :
	public Entity::Visitor // for parsing custom stim keyvalues from entities
{
public:
	// Tree model definition for a Stim/Response list
	struct Columns :
		public wxutil::TreeModel::ColumnRecord
	{
		Columns() :
			id(add(wxutil::TreeModel::Column::Integer)),
			caption(add(wxutil::TreeModel::Column::IconText)),
			name(add(wxutil::TreeModel::Column::String)),
			captionPlusID(add(wxutil::TreeModel::Column::String)),
			isCustom(add(wxutil::TreeModel::Column::Bool))
		{}

		wxutil::TreeModel::Column id;				// ID
		wxutil::TreeModel::Column caption;			// Caption String
		wxutil::TreeModel::Column name;				// Name
		wxutil::TreeModel::Column captionPlusID;	// The caption plus ID in brackets
		wxutil::TreeModel::Column isCustom;			// TRUE if the row is a custom stim type
	};

private:
	// The list of available stims
	StimTypeMap _stimTypes;

	// The empty stim.
	StimType _emptyStimType;

	// The list store for use in combo boxes
	Columns _columns;
	wxutil::TreeModel* _listStore;

public:
	/** greebo: Constructor, loads the Stim types from the registry.
	 */
	StimTypes();

	// Releases the TreeModel reference
	~StimTypes();

	/** greebo: Saves the custom stim types to the storage entity
	 */
	void save();

	/** greebo: Reloads the custom stim types from the map entities.
	 */
	void reload();

	/** greebo: Returns the reference to the internal stim type map
	 */
	StimTypeMap& getStimMap();

	/** greebo: Returns the StimType with the given ID
	 */
	StimType get(int id) const;

	/** greebo: Returns the StimType for the given name (STIM_FIRE)
	 */
	StimType get(const std::string& name) const;

	// Returns the numeric ID for the given stim type name
	// Returns -1 if nothing found
	int getIdForName(const std::string& name) const;

	/** greebo: Returns the name of the first available stimtype name.
	 */
	std::string getFirstName();

	/** greebo: Returns the GtkTreeIter pointing to the element
	 * 			named <name> located in the member _listStore.
	 */
	wxDataViewItem getIterForName(const std::string& name);

	// Get the liststore for use in combo boxes and treeviews
	const Columns& getColumns() const;
	wxutil::TreeModel* getListStore() const;

	// Load the stim type list into the choice array of the given combo box
	void populateComboBox(wxComboBox* combo) const;
	void populateComboBox(wxBitmapComboBox* combo) const;

	/** greebo: Entity::Visitor implementation. This parses the keyvalues
	 * 			for custom stim definitions.
	 */
	void visit(const std::string& key, const std::string& value);

	/** greebo: Retrieves the lowest available custom stim id.
	 */
	int getFreeCustomStimId();

	/** greebo: Adds a new stim type to the list and updates the liststore.
	 * 			Pass the relevant string properties like "name" as arguments.
	 */
	void add(int id,
			 const std::string& name,
			 const std::string& caption,
			 const std::string& description,
			 const std::string& icon,
			 bool custom);

	/** greebo: Updates the caption of the stimtype with the given id
	 * 			to <caption> and updates the internal list store.
	 */
	void setStimTypeCaption(int id, const std::string& caption);

	/** greebo: Removes the stim type with the given id.
	 * 			This works for custom stims only.
	 */
	void remove(int id);

	/**
	 * greebo: Retrieves the GtkTreeIter pointing at the row with the
	 * stim type with the given ID
	 */
	wxDataViewItem getIterForId(int id);
};
