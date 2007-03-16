#ifndef STIMTYPES_H_
#define STIMTYPES_H_

#include <map>
#include <string>

// Forward Declaration
typedef struct _GtkListStore GtkListStore; 

/** greebo: A simple StimType representation.
 */
struct StimType {
	std::string name;			// The name (STIM_FIRE)
	std::string caption;		// Nice format ("Fire")
	std::string description;	// Unused at the moment
};
typedef std::map<int, StimType> StimTypeMap;

class StimTypes
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
	
	/** greebo: Returns the Stim with the given ID
	 */
	StimType get(int id);
	
	// operator cast onto a GtkListStore, use this to pack the liststore
	operator GtkListStore* ();
};


#endif /*STIMTYPES_H_*/
