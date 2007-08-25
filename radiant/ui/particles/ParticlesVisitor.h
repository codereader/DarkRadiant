#ifndef PARTICLESVISITOR_H_
#define PARTICLESVISITOR_H_

#include "iparticles.h"

#include <gtk/gtkliststore.h>

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
	
public:
	
	/**
	 * Constructor.
	 */
	ParticlesVisitor(GtkListStore* store)
	: _store(store)
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
	}
};

}

#endif /*PARTICLESVISITOR_H_*/
