#ifndef SRENTITY_H_
#define SRENTITY_H_

#include <vector>
#include <string>
#include <gtk/gtktreemodel.h>
#include <boost/shared_ptr.hpp>

#include "StimResponse.h"
#include "StimTypes.h"

// Forward declaration
class Entity;
typedef struct _GtkListStore GtkListStore;

	namespace {
		const std::string ICON_STIM = "sr_stim.png";
		const std::string ICON_RESPONSE = "sr_response.png";
		
		enum {
			ID_COL,
			CLASS_COL,
			CAPTION_COL,
			ICON_COL,
			INHERIT_COL,
			IDSTR_COL,
			NUM_COLS
		};
	}

/** greebo: This is the representation of an entity holding S/R keys.
 * 			Use the load() and save() methods to load/save the spawnargs.
 * 			
 * 			The operator cast to GtkListStore* is available to pack
 * 			the data into a GtkTreeView.
 */
class SREntity
{
public:
	// These are the possible key names
	typedef std::vector<std::string> KeyList;
	
	// These are the int-indexed Stims/Responses belonging to an entity
	typedef std::map<int, StimResponse> StimResponseMap;
	
private:
	// The local lists of S/R and possible keys
	StimResponseMap _list;
	KeyList _keys;
	
	// The liststore representation
	GtkListStore* _listStore;
	
	// A collection of warnings regarding the parsing of the spawnargs
	std::string _warnings;
	
	// The helper class managing the various stim types
	StimTypes _stimTypes;
	
	// An empty stim
	StimResponse _emptyStimResponse;

public:
	SREntity(Entity* source);

	void load(Entity* source);
	void save(Entity* target);
	
	StimResponse& get(int id);
	
	operator GtkListStore* ();
	
	/** greebo: Sets the <key> of the SR with the given <id> to <value>
	 */
	void setProperty(int id, const std::string& key, const std::string& value);
	
	/** greebo: Updates the ListStore according to the 
	 * 			values of the current StimResponseMap <_list>
	 */
	void updateListStore();
	
	/** greebo: Helper to load the possible key names
	 * 			from the registry into the _keys list.
	 */
	void loadKeys();
	
	/** greebo: Returns the treeIter pointing to the row containing the 
	 * 			StimResponse with the given <id>
	 */
	GtkTreeIter getIterForId(int id);
};

typedef boost::shared_ptr<SREntity> SREntityPtr; 

#endif /*SRENTITY_H_*/
