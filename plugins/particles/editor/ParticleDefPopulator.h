#pragma once

#include "iparticles.h"
#include "ParticleEditor.h"

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

	const ParticleEditor::DefColumns& _columns;
	
public:

	/**
	 * Constructor.
	 */
	ParticlesVisitor(const Glib::RefPtr<Gtk::ListStore>& store,
					 const ParticleEditor::DefColumns& columns)
	: _store(store),
	  _columns(columns)
	{}

	/**
	 * Functor operator.
	 */
	void operator() (const particles::IParticleDef& def)
	{
		// Add the Def name to the list store
		Gtk::TreeModel::iterator iter = _store->append();

		Gtk::TreeModel::Row row = *iter;
		row[_columns.name] = def.getName();
	}
};

}
