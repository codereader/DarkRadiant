#include "DifficultySettings.h"

#include <gtk/gtktreestore.h>
#include "string/string.h"

namespace difficulty {

DifficultySettings::DifficultySettings(int level) :
	_level(level)
{}

int DifficultySettings::getLevel() const {
	return _level;
}

void DifficultySettings::clear() {
	_settings.clear();
	_iterMap.clear();
}

void DifficultySettings::updateTreeModel(GtkTreeStore* store) {
	gtk_tree_store_clear(store);

	for (SettingsMap::iterator i = _settings.begin(); i != _settings.end(); i++) {
		const std::string& className = i->first;
		const Setting& setting = *i->second;

		GtkTreeIter classIter = findOrInsertClassname(store, className);

		// Now insert the settings description into the map
		GtkTreeIter iter;
		gtk_tree_store_append(store, &iter, &classIter);
		gtk_tree_store_set(store, &iter, 
			COL_DESCRIPTION, setting.getDescString().c_str(), 
			COL_TEXTCOLOUR, setting.isDefault ? "#707070" : "black", 
			-1);
	}
}

std::string DifficultySettings::getParentClass(const std::string& className) {
	// Get the parent eclass
	IEntityClassPtr eclass = GlobalEntityClassManager().findClass(className);

	if (eclass == NULL) { 
		return ""; // Invalid!
	}

	EntityClassAttribute inheritAttr = eclass->getAttribute("inherit");
	return inheritAttr.value;
}

GtkTreeIter DifficultySettings::findOrInsertClassname(
	GtkTreeStore* store, const std::string& className)
{
	// Try to look up the classname in the tree
	TreeIterMap::iterator found = _iterMap.find(className);

	if (found != _iterMap.end()) {
		// Name exists, return this
		return found->second;
	}

	// This iter will hold the parent element, if such is found
	GtkTreeIter* parentIter(NULL);

	// Classname is not yet registered, walk up the inheritance tree
	std::string parentClassName = getParentClass(className);
	while (!parentClassName.empty()) {
		// Try to look up the classname in the tree
		TreeIterMap::iterator found = _iterMap.find(parentClassName);

		if (found != _iterMap.end()) {
			parentIter = &found->second;
			break;
		}

		parentClassName = getParentClass(parentClassName);
	}

	// Insert the map, using the found iter (or NULL, if nothing was found)
	GtkTreeIter inserted = insertClassName(store, className, parentIter);

	// Remember the iter
	_iterMap.insert(TreeIterMap::value_type(className, inserted));

	return inserted;
}

GtkTreeIter DifficultySettings::insertClassName(
	GtkTreeStore* store, const std::string& className, GtkTreeIter* parent)
{
	GtkTreeIter iter;
	gtk_tree_store_append(store, &iter, parent);
	gtk_tree_store_set(store, &iter, 
		COL_DESCRIPTION, className.c_str(), 
		COL_TEXTCOLOUR, "black", 
		-1);

	return iter;
}

void DifficultySettings::parseFromEntityDef(const IEntityClassPtr& def) {
	// Construct the prefix for the desired difficulty level
	std::string diffPrefix = "diff_" + intToStr(_level) + "_";
	std::string prefix = diffPrefix + "change_";

	EntityClassAttributeList spawnargs = def->getAttributeList(prefix);

	for (EntityClassAttributeList::iterator i = spawnargs.begin();
		 i != spawnargs.end(); i++)
	{
		EntityClassAttribute& attr = *i;

		if (attr.value.empty()) {
			continue; // empty spawnarg attribute => invalid
		}

		// Get the index from the string's tail
		std::string indexStr = attr.name.substr(prefix.length());
		int index = strToInt(indexStr);

		const EntityClassAttribute& classAttr = def->getAttribute(diffPrefix + "class_" + indexStr);
		const EntityClassAttribute& argAttr = def->getAttribute(diffPrefix + "arg_" + indexStr);
		
		SettingPtr setting(new Setting);
		setting->className = classAttr.value;
		setting->spawnArg = attr.value;
		setting->argument = argAttr.value;

		// This has been parsed from the default entityDef
		setting->isDefault = true;

		// Interpret/parse the argument string
		setting->parseAppType();

		// Insert the parsed setting into our local map
		_settings.insert(SettingsMap::value_type(setting->className, setting));
	}
}

void DifficultySettings::parseFromMapEntity(Entity* entity) {
	// Construct the prefix for the desired difficulty level
	std::string diffPrefix = "diff_" + intToStr(_level) + "_";
	std::string prefix = diffPrefix + "change_";

	Entity::KeyValuePairs spawnargs = entity->getKeyValuePairs(prefix);

	for (Entity::KeyValuePairs::iterator i = spawnargs.begin();
		 i != spawnargs.end(); i++)
	{
		const std::string& key = i->first;
		const std::string& value = i->second;

		if (value.empty()) {
			continue; // empty spawnarg attribute => invalid
		}

		// Get the index from the string's tail
		std::string indexStr = key.substr(prefix.length());
		int index = strToInt(indexStr);

		SettingPtr setting(new Setting);
		setting->className = entity->getKeyValue(diffPrefix + "class_" + indexStr);
		setting->spawnArg = value;
		setting->argument = entity->getKeyValue(diffPrefix + "arg_" + indexStr);

		// This has been parsed from the default entityDef
		setting->isDefault = false;

		// Interpret/parse the argument string
		setting->parseAppType();

		// Insert the parsed setting into our local map
		_settings.insert(SettingsMap::value_type(setting->className, setting));
	}
}

} // namespace difficulty
