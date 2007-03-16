#ifndef SRENTITY_H_
#define SRENTITY_H_

#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>

#include "StimResponse.h"

// Forward declaration
class Entity;
typedef struct _GtkListStore GtkListStore;

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
	
	// These are the Stims/Responses belonging to an entity
	typedef std::vector<StimResponse> SRList;
	
private:
	// The local lists of S/R and possible keys
	SRList _list;
	KeyList _keys;
	
	// The liststore representation
	GtkListStore* _listStore;

public:
	SREntity(Entity* source);

	void load(Entity* source);
	void save(Entity* target);
		
	operator GtkListStore* ();
	
	/** greebo: Helper to load the possible key names
	 * 			from the registry into the _keys list.
	 */
	void loadKeys();
};

typedef boost::shared_ptr<SREntity> SREntityPtr; 

#endif /*SRENTITY_H_*/
