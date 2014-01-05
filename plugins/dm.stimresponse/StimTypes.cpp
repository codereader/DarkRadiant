#include "StimTypes.h"

#include "iuimanager.h"
#include "itextstream.h"
#include "string/string.h"
#include "gtkutil/TreeModel.h"
#include "entitylib.h"
#include "registry/registry.h"
#include "SREntity.h"
#include "gamelib.h"
#include "i18n.h"
#include "igame.h"

#include <boost/algorithm/string/predicate.hpp>

namespace {
	const std::string RKEY_STIM_DEFINITIONS =
		"/stimResponseSystem/stims//stim";
	const std::string GKEY_STORAGE_ECLASS =
		"/stimResponseSystem/customStimStorageEClass";
	const std::string GKEY_STORAGE_PREFIX =
		"/stimResponseSystem/customStimKeyPrefix";
	const std::string GKEY_LOWEST_CUSTOM_STIM_ID =
		"/stimResponseSystem/lowestCustomStimId";
	const std::string RKEY_SHOW_STIM_TYPE_IDS =
		"user/ui/stimResponseEditor/showStimTypeIDs";

	/* greebo: Finds an entity with the given classname
	 */
	Entity* findEntityByClass(const std::string& className) {
		// Instantiate a walker to find the entity
		EntityNodeFindByClassnameWalker walker(className);

		// Walk the scenegraph
		GlobalSceneGraph().root()->traverse(walker);

		return walker.getEntity();
	}

	// Helper visitor class to remove custom stim definitions from
	// the storage entity. First, all the keys are gathered and
	// on destruction the keys are deleted. The deletion may not
	// happen during the visit process (due to iterators becoming invalid).
	class CustomStimRemover :
		public Entity::Visitor
	{
		// This list will be populated with all the keys that
		// should be removed.
		typedef std::vector<std::string> RemoveList;
		RemoveList _removeList;

		Entity* _entity;

	public:
		CustomStimRemover(Entity* entity) :
			_entity(entity)
		{}

		~CustomStimRemover() {
			// Delete all the keys that are tagged for deletion
			for (std::size_t i = 0; i < _removeList.size(); ++i)
			{
				_entity->setKeyValue(_removeList[i], "");
			}
		}

		void visit(const std::string& key, const std::string& value) {
			std::string prefix = game::current::getValue<std::string>(GKEY_STORAGE_PREFIX);

			if (boost::algorithm::starts_with(key, prefix)) {
				// We have a match, add the key to the removal list
				_removeList.push_back(key);
			}
		}
	};
}

StimTypes::StimTypes() :
	_listStore(Gtk::ListStore::create(_columns))
{}

void StimTypes::reload()
{
	_stimTypes.clear();
	_listStore->clear();

	// Find all the relevant nodes
	xml::NodeList stimNodes = GlobalGameManager().currentGame()->getLocalXPath(RKEY_STIM_DEFINITIONS);

	for (std::size_t i = 0; i < stimNodes.size(); ++i)
	{
		// Add the new stim type
		add(string::convert<int>(stimNodes[i].getAttributeValue("id")),
			stimNodes[i].getAttributeValue("name"),
			stimNodes[i].getAttributeValue("caption"),
			stimNodes[i].getAttributeValue("description"),
			stimNodes[i].getAttributeValue("icon"),
			false	// non-custom stim
		);
	}

	// Load the custom stims from the storage entity
	std::string storageEClass = game::current::getValue<std::string>(GKEY_STORAGE_ECLASS);
	Entity* storageEntity = findEntityByClass(storageEClass);

	if (storageEntity != NULL)
	{
		// Visit each keyvalue with the <self> class as visitor
		storageEntity->forEachKeyValue(*this);
	}
}

void StimTypes::save()
{
	// Find the storage entity
	std::string storageEClass = game::current::getValue<std::string>(GKEY_STORAGE_ECLASS);
	Entity* storageEntity = findEntityByClass(storageEClass);

	if (storageEntity != NULL)
	{
		std::string prefix = game::current::getValue<std::string>(GKEY_STORAGE_PREFIX);

		// Clean the storage entity from any previous definitions
		{
			// Instantiate a visitor to gather the keys to delete
			CustomStimRemover remover(storageEntity);
			// Visit each keyvalue with the <self> class as visitor
			storageEntity->forEachKeyValue(remover);
			// Scope ends here, the keys are deleted now
			// as the CustomStimRemover gets destructed
		}

		// Now store all custom stim types to the storage entity
		for (StimTypeMap::iterator i = _stimTypes.begin(); i != _stimTypes.end(); ++i)
		{
			StimType& s = i->second;
			std::string idStr = string::to_string(i->first);

			if (s.custom) {
				// spawnarg is something like "editor_dr_stim_1002" => "MyStim"
				storageEntity->setKeyValue(prefix + idStr, s.caption);
			}
		}
	}
}

void StimTypes::remove(int id)
{
	StimTypeMap::iterator found = _stimTypes.find(id);

	if (found != _stimTypes.end())
	{
		// Erase the item from the map
		_stimTypes.erase(found);

		// Erase the row in the liststore
		Gtk::TreeModel::iterator iter = getIterForId(id);
		_listStore->erase(iter);
	}
}

Gtk::TreeModel::iterator StimTypes::getIterForId(int id)
{
	// Setup the selectionfinder to search for the id
	gtkutil::TreeModel::SelectionFinder finder(id, _columns.id.index());

	_listStore->foreach_iter(
		sigc::mem_fun(finder, &gtkutil::TreeModel::SelectionFinder::forEach));

	return finder.getIter();
}

void StimTypes::setStimTypeCaption(int id, const std::string& caption)
{
	StimTypeMap::iterator found = _stimTypes.find(id);

	if (found != _stimTypes.end())
	{
		_stimTypes[id].caption = caption;

		// Combine the ID and the caption
		std::string captionPlusId = caption;
		bool showStimTypeIds = registry::getValue<bool>(RKEY_SHOW_STIM_TYPE_IDS);
		captionPlusId += showStimTypeIds ? " (" + string::to_string(id) + ")" : "";

		// Update the list store
		Gtk::TreeModel::Row row = *getIterForId(id);

		row[_columns.caption] = caption;
		row[_columns.captionPlusID] = captionPlusId;
	}
}

void StimTypes::add(int id,
					const std::string& name,
					const std::string& caption,
					const std::string& description,
					const std::string& icon,
					bool custom)
{
	StimType newStimType;
	newStimType.name = name;
	newStimType.caption = caption;
	newStimType.description = description;
	newStimType.icon = icon;
	newStimType.custom = custom;

	// Add the stim to the map
	_stimTypes[id] = newStimType;

	// Combine the ID and the caption
	std::string captionPlusId = caption;
	bool showStimTypeIds = registry::getValue<bool>(RKEY_SHOW_STIM_TYPE_IDS);
	captionPlusId += showStimTypeIds ? " (" + string::to_string(id) + ")" : "";

	Gtk::TreeModel::Row row = *_listStore->append();

	row[_columns.id] = id;
	row[_columns.caption] = _stimTypes[id].caption;
	row[_columns.captionPlusID] = captionPlusId;
	row[_columns.icon] = GlobalUIManager().getLocalPixbufWithMask(newStimType.icon);
	row[_columns.name] = _stimTypes[id].name;
	row[_columns.isCustom] = custom;
}

void StimTypes::visit(const std::string& key, const std::string& value)
{
	std::string prefix = game::current::getValue<std::string>(GKEY_STORAGE_PREFIX);
	int lowestCustomId = game::current::getValue<int>(GKEY_LOWEST_CUSTOM_STIM_ID);

	if (boost::algorithm::starts_with(key, prefix))
	{
		// Extract the stim name from the key (the part after the prefix)
		std::string idStr = key.substr(prefix.size());
		int id = string::convert<int>(idStr);
		std::string stimCaption= value;

		if (id < lowestCustomId)
		{
			rError() << "Warning: custom stim Id " << id << " is lower than "
								<< lowestCustomId << "\n";
		}

		// Add this as new stim type
		add(id,
			idStr,	// The name is the id in string format: e.g. "1002"
			stimCaption,	// The caption
			_("Custom Stim"),
			ICON_CUSTOM_STIM,
			true	// custom stim
		);
	}
}

const StimTypes::Columns& StimTypes::getColumns() const
{
	return _columns;
}

Glib::RefPtr<Gtk::TreeModel> StimTypes::getListStore() const
{
	return _listStore;
}

StimTypeMap& StimTypes::getStimMap()
{
	return _stimTypes;
}

int StimTypes::getFreeCustomStimId()
{
	int freeId = game::current::getValue<int>(GKEY_LOWEST_CUSTOM_STIM_ID);

	StimTypeMap::iterator found = _stimTypes.find(freeId);

	while (found != _stimTypes.end())
	{
		freeId++;
		found = _stimTypes.find(freeId);
	}

	return freeId;
}

Gtk::TreeModel::iterator StimTypes::getIterForName(const std::string& name)
{
	// Setup the selectionfinder to search for the name string
	gtkutil::TreeModel::SelectionFinder finder(name, _columns.name.index());

	_listStore->foreach_iter(
		sigc::mem_fun(finder, &gtkutil::TreeModel::SelectionFinder::forEach));

	return finder.getIter();
}

StimType StimTypes::get(int id)
{
	StimTypeMap::iterator i = _stimTypes.find(id);

	return i != _stimTypes.end() ? i->second : _emptyStimType;
}

std::string StimTypes::getFirstName()
{
	StimTypeMap::iterator i = _stimTypes.begin();

	return (i != _stimTypes.end()) ? i->second.name : "noname";
}

StimType StimTypes::get(const std::string& name)
{
	for (StimTypeMap::iterator i = _stimTypes.begin(); i!= _stimTypes.end(); ++i)
	{
		if (i->second.name == name)
		{
			return i->second;
		}
	}

	return _emptyStimType; // Nothing found
}
