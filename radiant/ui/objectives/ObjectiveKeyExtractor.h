#ifndef OBJECTIVEKEYEXTRACTOR_H_
#define OBJECTIVEKEYEXTRACTOR_H_

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

namespace ui
{

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
	typedef std::map<int, GtkTreeIter*> IterMap; 
	IterMap _parentMap;
	
private:

	// Get the iter for a numbered objective, creating it if necessary
	GtkTreeIter* getParent(int num) {
		
		// Locate iter if it exists
		IterMap::iterator i = _parentMap.find(num);
		if (i != _parentMap.end())
			return i->second;
			
		// Otherwise create at the tree root
		GtkTreeIter* gtkIter = new GtkTreeIter();
		std::string objText = "Objective " 
							  + boost::lexical_cast<std::string>(num);
		
		gtk_tree_store_append(_store, gtkIter, NULL);
		gtk_tree_store_set(_store, gtkIter, 0, objText.c_str(), -1);
				
		// Add to cache and return
		_parentMap.insert(IterMap::value_type(num, gtkIter));
		return gtkIter;
	}
	
public:

	/**
	 * Constructor. Set the tree store to populate.
	 */
	ObjectiveKeyExtractor(GtkTreeStore* store)
	: _store(store)
	{ }
	
	/**
	 * Required visit function.
	 */
	void visit(const std::string& key, const std::string& value) {
		
//		// Quick discard of any non-objective keys
//		if (key.substr(0, 3) != "obj")
//			return;
//			
//		// Use a boost::regex to extract the objective number.
//		static const boost::regex e("obj(\\d+)_(.*)");
//		boost::smatch result;
//		if (!boost::regex_match(key, result, e))
//			return;
//		
//		// The cmatch result should now contain the objective number and the
//		// standard objective key.
//		int num = boost::lexical_cast<int>(result[1]);
//		std::string objKey = result[2];
//		
//		GtkTreeIter iter;
//		gtk_tree_store_append(_store, &iter, getParent(num));
//		gtk_tree_store_set(_store, &iter, 
//						   0, objKey.c_str(), 1, value.c_str(), -1);
	}
};

}

#endif /*OBJECTIVEKEYEXTRACTOR_H_*/
