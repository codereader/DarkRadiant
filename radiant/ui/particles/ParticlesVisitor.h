#ifndef PARTICLESVISITOR_H_
#define PARTICLESVISITOR_H_

#include "iparticles.h"

#include <gtk/gtkliststore.h>

#include <map>

namespace ui
{

/**
 * Visitor class to retrieve particle system names and add them to a 
 * GtkListStore.
 */
class ParticlesVisitor
{
	// List store to populate
	GtkListStore* _store;

	// Map of iters for fast lookup
	typedef std::map<std::string, GtkTreeIter*> IterMap;
	IterMap& _iterMap;
	
public:
	
	/**
	 * Constructor.
	 */
	ParticlesVisitor(GtkListStore* store, IterMap& map)
	: _store(store), _iterMap(map)
	{ }
	
	/**
	 * Functor operator.
	 */
	void operator() (const IParticleDef& def) {
		
		// Add the ".prt" extension to the name fo display in the list
		std::string prtName = def.getName() + ".prt";
		
		// Add the Def name to the list store
		GtkTreeIter iter;
		gtk_list_store_append(_store, &iter);
		gtk_list_store_set(_store, &iter, 0, prtName.c_str(), -1);
		
		// Save the iter in the iter map
		GtkTreeIter* permanentIter = gtk_tree_iter_copy(&iter);
		_iterMap.insert(IterMap::value_type(prtName, permanentIter));
	}
};

}

#endif /*PARTICLESVISITOR_H_*/
