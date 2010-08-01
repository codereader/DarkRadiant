#ifndef PARTICLESVISITOR_H_
#define PARTICLESVISITOR_H_

#include "iparticles.h"
#include "ParticlesChooser.h"

namespace ui
{

/**
 * Visitor class to retrieve particle system names and add them to a 
 * GtkListStore.
 */
class ParticlesVisitor
{
private:
	// List store to populate
	Glib::RefPtr<Gtk::ListStore> _store;

	const ParticlesChooser::ListColumns& _columns;

	// Map of iters for fast lookup
	ParticlesChooser::IterMap& _iterMap;
	
public:
	
	/**
	 * Constructor.
	 */
	ParticlesVisitor(const Glib::RefPtr<Gtk::ListStore>& store, 
					 const ParticlesChooser::ListColumns& columns, 
					 ParticlesChooser::IterMap& map)
	: _store(store), 
	  _columns(columns),
	  _iterMap(map)
	{}
	
	/**
	 * Functor operator.
	 */
	void operator() (const IParticleDef& def)
	{
		// Add the ".prt" extension to the name fo display in the list
		std::string prtName = def.getName() + ".prt";
		
		// Add the Def name to the list store
		Gtk::TreeModel::iterator iter = _store->append();

		Gtk::TreeModel::Row row = *iter;
		row[_columns.name] = prtName;
		
		// Save the iter in the iter map
		_iterMap.insert(ParticlesChooser::IterMap::value_type(prtName, iter));
	}
};

}

#endif /*PARTICLESVISITOR_H_*/
