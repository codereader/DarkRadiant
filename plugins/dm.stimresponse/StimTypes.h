#ifndef STIMTYPES_H_
#define STIMTYPES_H_

#include <map>
#include <string>
#include "ientity.h"

// Forward Declaration
typedef struct _GtkListStore GtkListStore; 
typedef struct _GtkTreeIter GtkTreeIter;

	namespace {
		enum {
		  ST_ID_COL,
		  ST_CAPTION_COL,
		  ST_ICON_COL,
		  ST_NAME_COL,
		  ST_CAPTION_PLUS_ID_COL,	// The caption plus ID in brackets
		  ST_CUSTOM_COL,	// TRUE if the row is a custom stim type
		  ST_NUM_COLS
		};
	}

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
	// The list of available stims 
	StimTypeMap _stims;
	
	// The empty stim.
	StimType _emptyStimType;
	
	// The GTK list store for use in combo boxes
	GtkListStore* _listStore;

public:
	/** greebo: Constructor, loads the Stim types from the registry.
	 */
	StimTypes();
	
	/** greebo: Returns the reference to the internal stim type map
	 */
	StimTypeMap& getStimMap();
	
	/** greebo: Returns the StimType with the given ID
	 */
	StimType get(int id);
	
	/** greebo: Returns the StimType for the given name (STIM_FIRE)
	 */
	StimType get(const std::string& name);
	
	/** greebo: Returns the name of the first available stimtype name.
	 */
	std::string getFirstName();
	
	/** greebo: Returns the GtkTreeIter pointing to the element
	 * 			named <name> located in the member _listStore.
	 */
	GtkTreeIter getIterForName(const std::string& name);
	
	// operator cast onto a GtkListStore, use this to pack the liststore
	operator GtkListStore* ();
	
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
	
	/** greebo: Retrieves the GtkTreeIter pointing at the row with the
	 * 			stim type with the given ID
	 */
	GtkTreeIter getIterForId(int id);
};


#endif /*STIMTYPES_H_*/
