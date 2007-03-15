#ifndef OBJECTIVEKEYEXTRACTOR_H_
#define OBJECTIVEKEYEXTRACTOR_H_

#include "gtkutil/image.h"

#include <boost/lexical_cast.hpp>
#include <map>

namespace ui
{

/* CONSTANTS */
namespace {
	
	const char* OBJ_ICON = "objectives16.png";
	
}

/**
 * Entity Visitor which extracts objective keyvalues (of the form "obj<n>_blah")
 * and populates the given GtkTreeStore with appropriate information.
 */
class ObjectiveKeyExtractor
: public Entity::Visitor
{
	// Tree store to populate
	GtkTreeStore* _store;

	// Map of iters for parent objectives
	typedef std::map<int, boost::shared_ptr<GtkTreeIter> > IterMap; 
	IterMap _parentMap;
	
	// Map of display names from objective standard keys (e.g. 
	// "desc" -> "Description").
	typedef std::map<std::string, std::string> StringMap;
	StringMap _displayNames;
	
private:

	// Get the iter for a numbered objective, creating it if necessary
	boost::shared_ptr<GtkTreeIter> getParent(int num) {
		
		// Locate iter if it exists
		IterMap::iterator i = _parentMap.find(num);
		if (i != _parentMap.end())
			return i->second;
			
		// Otherwise create at the tree root
		boost::shared_ptr<GtkTreeIter> gtkIter(new GtkTreeIter());
		std::string objText = "Objective " 
							  + boost::lexical_cast<std::string>(num);
		
		gtk_tree_store_append(_store, gtkIter.get(), NULL);
		gtk_tree_store_set(_store, gtkIter.get(), 
						   0, objText.c_str(), 
						   1, "",
						   2, gtkutil::getLocalPixbuf(OBJ_ICON),
						   -1);
				
		// Add to cache and return
		_parentMap.insert(IterMap::value_type(num, gtkIter));
		return gtkIter;
	}
	
	// Get a display name for the given objective standard key.
	std::string getDisplayName(const std::string& key) {
		StringMap::iterator i = _displayNames.find(key);
		if (i != _displayNames.end())
			return i->second;
		else
			return key;		
	}
	
public:

	/**
	 * Constructor. Set the tree store to populate.
	 */
	ObjectiveKeyExtractor(GtkTreeStore* store)
	: _store(store)
	{ 
		// Populate the display name map.
		_displayNames["desc"] = "Description";
		_displayNames["state"] = "State";
		_displayNames["ongoing"] = "Ongoing";
		_displayNames["mandatory"] = "Mandatory";
		_displayNames["visible"] = "Visible";
		_displayNames["irreversible"] = "Irreversible";
	}
	
	/**
	 * Required visit function.
	 */
	void visit(const std::string& key, const std::string& value) {
		
		// Quick discard of any non-objective keys
		if (key.substr(0, 3) != "obj")
			return;
			
		// Extract the objective number, which should exist between key[3] and
		// the leftmost underscore
		int uPos = key.find("_");
		std::string sObjNum = key.substr(3, uPos - 3);
		int num = boost::lexical_cast<int>(sObjNum);

		// Chop of the "obj1_" prefix to get the standard parameter name
		std::string objParm = getDisplayName(key.substr(uPos + 1));

		GtkTreeIter iter;
		gtk_tree_store_append(_store, &iter, getParent(num).get());
		gtk_tree_store_set(_store, &iter, 
						   0, objParm.c_str(), 
						   1, value.c_str(), -1);
	}
};

}

#endif /*OBJECTIVEKEYEXTRACTOR_H_*/
