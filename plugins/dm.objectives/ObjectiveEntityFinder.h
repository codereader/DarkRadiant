#pragma once

#include "ObjectiveEntity.h"

#include "i18n.h"

#include <string>
#include <boost/format.hpp>
#include <gtkmm/liststore.h>

namespace objectives
{

struct ObjectiveEntityListColumns :
	public Gtk::TreeModel::ColumnRecord
{
	ObjectiveEntityListColumns()
	{
		add(displayName);
		add(startActive);
		add(entityName);
	}

	Gtk::TreeModelColumn<Glib::ustring> displayName;
	Gtk::TreeModelColumn<bool> startActive;
	Gtk::TreeModelColumn<Glib::ustring> entityName;
};

/**
 * Visitor class to locate and list any <b>atdm:target_addobjectives</b> entities in
 * the current map.
 *
 * The ObjectiveEntityFinder will visit each scenegraph node in turn, as per the
 * behaviour of a scenegraph walker. The classname of each entity visited is
 * tested against a given value (passed in during construction) which identifies
 * it as an Objectives entity, and if the test is successful, the entity's
 * details are added to the target ObjectiveEntityMap and GtkListStore objects
 * to be populated.
 *
 * The ObjectiveEntityFinder also keeps a reference to the worldspawn entity so
 * that the "activate at start" status can be determined (the worldspawn targets
 * any objective entities that should be active at start).
 */
class ObjectiveEntityFinder
: public scene::NodeVisitor
{
	// List of names of entity class we are looking for
	std::vector<std::string> _classNames;

	// GtkListStore to populate with results
	const ObjectiveEntityListColumns& _columns;
	Glib::RefPtr<Gtk::ListStore> _store;

	// ObjectiveEntityMap which we also populate
	ObjectiveEntityMap& _map;

	// Worldspawn entity
	Entity* _worldSpawn;

public:

	/**
	 * Construct a visitor to populate the given store and ObjectiveEntityMap.
	 *
	 * The GtkListStore provided must contain three columns. The first column
	 * is a G_TYPE_STRING containing the display name of the Objectives entity,
	 * which is constructed from the real entity name plus the origin in
	 * brackets for convenience purposes. The second column is a G_TYPE_BOOL
	 * which is set to TRUE if the entity is activated at start, and FALSE
	 * otherwise. The third column is a G_TYPE_STRING containing the raw entity
	 * name in the map.
	 *
	 * @param st
	 * The GtkListStore to populate.
	 *
	 * @param map
	 * The ObjectiveEntityMap to populate.
	 *
	 * @param classname
	 * The text classname used to identify an Objectives entity.
	 */
	ObjectiveEntityFinder(const Glib::RefPtr<Gtk::ListStore>& st,
						  const ObjectiveEntityListColumns& columns,
						  ObjectiveEntityMap& map,
						  const std::vector<std::string>& classnames)
	: _classNames(classnames),
	  _columns(columns),
	  _store(st),
	  _map(map),
	  _worldSpawn(NULL)
	{ }

	/**
	 * Return a pointer to the worldspawn entity. This could potentially be
	 * NULL if a worldspawn entity was not found during visitation.
	 */
	Entity* getWorldSpawn()
	{
		return _worldSpawn;
	}

	/**
	 * @see scene::NodeVisitor::pre()
	 */
	bool pre(const scene::INodePtr& node);

};

}

