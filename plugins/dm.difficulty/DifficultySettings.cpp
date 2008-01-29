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
}

void DifficultySettings::updateTreeModel(GtkTreeStore* store) {
	gtk_tree_store_clear(store);

	for (SettingsMap::iterator i = _settings.begin(); i != _settings.end(); i++) {
		const std::string& className = i->first;
		const Setting& setting = *i->second;

		GtkTreeIter iter;
		gtk_tree_store_append(store, &iter, NULL);
		gtk_tree_store_set(store, &iter, 0, className.c_str(), -1);
	}
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
		// Interpret/parse the argument string
		setting->parseAppType();

		// Insert the parsed setting into our local map
		_settings.insert(SettingsMap::value_type(setting->className, setting));
	}
}

} // namespace difficulty
