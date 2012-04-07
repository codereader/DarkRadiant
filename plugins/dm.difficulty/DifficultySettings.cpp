#include "DifficultySettings.h"

#include "string/convert.h"
#include "eclass.h"

namespace difficulty
{

DifficultySettings::DifficultySettings(int level) :
    _level(level),
    _store(Gtk::TreeStore::create(_columns))
{}

const DifficultySettings::TreeModelColumns& DifficultySettings::getColumns() const
{
    return _columns;
}

const Glib::RefPtr<Gtk::TreeStore>&  DifficultySettings::getTreeStore() const
{
    return _store;
}

int DifficultySettings::getLevel() const
{
    return _level;
}

void DifficultySettings::clear()
{
    _settings.clear();
    _settingIds.clear();
    _iterMap.clear();
}

SettingPtr DifficultySettings::getSettingById(int id) const {
    // Search all stored settings matching this classname
    SettingIdMap::const_iterator found = _settingIds.find(id);

    if (found != _settingIds.end()) {
        return found->second;
    }

    return SettingPtr(); // not found
}

SettingPtr DifficultySettings::findOrCreateOverrule(const SettingPtr& existing)
{
    // Get the inheritancekey needed to lookup the classname
    std::string inheritanceKey = getInheritanceKey(existing->className);

    // Check if there is already an override active for the <existing> setting
    for (SettingsMap::iterator i = _settings.find(inheritanceKey);
         i != _settings.upper_bound(inheritanceKey) && i != _settings.end();
         ++i)
    {
        // Avoid self==self comparisons
        if (i->second != existing && i->second->spawnArg == existing->spawnArg) {
            // spawnarg is set on a different setting, check if it is non-default
            if (!i->second->isDefault) {
                // non-default, overriding setting, return it
                return i->second;
            }
        }
    }

    // No overriding setting found, create a new one
    SettingPtr setting = createSetting(existing->className);
    setting->spawnArg = existing->spawnArg;
    setting->isDefault = false;
    setting->appType = Setting::EAssign;

    return setting;
}

int DifficultySettings::save(int id, const SettingPtr& setting) {
    if (id != -1) {
        // We're dealing with an existing setting, fetch it
        SettingPtr existing = getSettingById(id);

        if (existing == NULL) {
            return -1;
        }

        if (existing->isDefault) {
            // We're trying to save a default setting, go into override mode

            if (*setting == *existing) {
                // New settings are identical to the existing ones, skip
                return existing->id;
            }

            // Create a new setting
            SettingPtr overrule = findOrCreateOverrule(existing);
            // Transfer the argument/appType into the new setting
            overrule->argument = setting->argument;
            overrule->appType = setting->appType;
            return overrule->id;
        }
        else {
            // TODO: Check, if there is another, existing setting which we
            // override with this one and whether they are the same

            // Copy the settings over to the existing setting
            *existing = *setting;
            return existing->id;
        }
    }
    else {
        // No setting highlighted, create a new one
        SettingPtr newSetting = createSetting(setting->className);
        // Copy the settings over
        *newSetting = *setting;
        newSetting->isDefault = false;
        return newSetting->id;
    }
}

void DifficultySettings::deleteSetting(int id)
{
    for (SettingsMap::iterator i = _settings.begin(); i != _settings.end(); ++i)
    {
        if (i->second->id == id)
        {
            // Found it, remove it from the tree and all maps
            _store->erase(i->second->iter);

            _settings.erase(i);
            _settingIds.erase(id);
            break;
        }
    }

    // Override settings might have been changed, update the model
    updateTreeModel();
}

void DifficultySettings::updateTreeModel()
{
    // Go through the settings and check the corresponding iters in the tree
    for (SettingsMap::iterator i = _settings.begin(); i != _settings.end(); ++i)
    {
        const std::string& className = i->second->className;
        Setting& setting = *i->second;

        // Ensure that the classname is in the map
        Gtk::TreeModel::iterator classIter = findOrInsertClassname(className);

        if (!_store->iter_is_valid(setting.iter))
        {
            // No iter corresponding to this setting yet, insert it
            setting.iter = classIter ? _store->append(classIter->children()) : _store->append();
        }

        Gtk::TreeModel::Row row = *(setting.iter);

        row[_columns.description] = setting.getDescString();
        row[_columns.colour] = setting.isDefault ? "#707070" : "black";
        row[_columns.classname] = setting.className;
        row[_columns.settingId] = setting.id;
        row[_columns.isOverridden] = isOverridden(i->second);
    }
}

void DifficultySettings::clearTreeModel()
{
    _iterMap.clear();
    _store->clear();
}

void DifficultySettings::refreshTreeModel()
{
    clearTreeModel();
    updateTreeModel();
}

bool DifficultySettings::isOverridden(const SettingPtr& setting)
{
    if (!setting->isDefault) {
        return false; // not a default setting, return false
    }

    // Get the inheritancekey needed to lookup the classname
    std::string inheritanceKey = getInheritanceKey(setting->className);

    // Search all other settings for the same className/spawnArg combination
    for (SettingsMap::iterator i = _settings.find(inheritanceKey);
         i != _settings.upper_bound(inheritanceKey) && i != _settings.end();
         ++i)
    {
        // Avoid self==self comparisons
        if (i->second != setting && i->second->spawnArg == setting->spawnArg) {
            // spawnarg is set on a different setting, check if it is non-default
            if (!i->second->isDefault) {
                // non-default, overriding setting, return true
                return true;
            }
        }
    }

    return false;
}

std::string DifficultySettings::getParentClass(const std::string& className) {
    // Get the parent eclass
    IEntityClassPtr eclass = GlobalEntityClassManager().findClass(className);

    if (eclass == NULL) {
        return ""; // Invalid!
    }

    EntityClassAttribute inheritAttr = eclass->getAttribute("inherit");
    return inheritAttr.getValue();
}

Gtk::TreeModel::iterator DifficultySettings::findOrInsertClassname(const std::string& className)
{
    // Try to look up the classname in the tree
    TreeIterMap::iterator found = _iterMap.find(className);

    if (found != _iterMap.end())
    {
        // Name exists, return this
        return found->second;
    }

    // This iter will hold the parent element, if such is found
    Gtk::TreeModel::iterator parentIter;

    // Classname is not yet registered, walk up the inheritance tree
    std::string parentClassName = getParentClass(className);
    while (!parentClassName.empty())
    {
        // Try to look up the classname in the tree
        TreeIterMap::iterator found = _iterMap.find(parentClassName);

        if (found != _iterMap.end())
        {
            parentIter = found->second;
            break;
        }

        parentClassName = getParentClass(parentClassName);
    }

    // Insert the map, using the found iter (or NULL, if nothing was found)
    Gtk::TreeModel::iterator inserted = insertClassName(className, parentIter);

    // Remember the iter
    _iterMap.insert(TreeIterMap::value_type(className, inserted));

    return inserted;
}

Gtk::TreeModel::iterator DifficultySettings::insertClassName(const std::string& className, Gtk::TreeModel::iterator parent)
{
    Gtk::TreeModel::iterator iter = parent ? _store->append(parent->children()) : _store->append();
    Gtk::TreeModel::Row row = *iter;

    row[_columns.description] = className;
    row[_columns.colour] = "black";
    row[_columns.classname] = className;
    row[_columns.settingId] = -1;
    row[_columns.isOverridden] = false;

    return iter;
}

std::string DifficultySettings::getInheritanceKey(const std::string& className)
{
    if (className.empty()) return "";

    IEntityClassPtr eclass = GlobalEntityClassManager().findClass(className);

    // Get the inheritance chain of this class
    std::list<std::string> inheritanceChain;
    for (const IEntityClass* currentClass = eclass.get();
         currentClass != NULL;
         currentClass = currentClass->getParent())
    {
        inheritanceChain.push_front(currentClass->getName());
    }

    // Build the inheritance key
    std::string inheritanceKey;
    for (std::list<std::string>::const_iterator c = inheritanceChain.begin();
         c != inheritanceChain.end();
         c++)
    {
        inheritanceKey += (inheritanceKey.empty()) ? "" : "_";
        inheritanceKey += *c;
    }

    return inheritanceKey;
}

SettingPtr DifficultySettings::createSetting(const std::string& className)
{
    SettingPtr setting(new Setting);
    setting->className = className;

    // Insert the parsed setting into our local map
    _settings.insert(SettingsMap::value_type(getInheritanceKey(className), setting));
    _settingIds.insert(SettingIdMap::value_type(setting->id, setting));

    return setting;
}

void DifficultySettings::parseFromEntityDef(const IEntityClassPtr& def)
{
    // Construct the prefix for the desired difficulty level
    std::string diffPrefix = "diff_" + string::to_string(_level) + "_";
    std::string prefix = diffPrefix + "change_";

    eclass::AttributeList spawnargs = eclass::getSpawnargsWithPrefix(*def, prefix);

    for (eclass::AttributeList::iterator i = spawnargs.begin();
         i != spawnargs.end(); ++i)
    {
        EntityClassAttribute& attr = *i;

        if (attr.getValue().empty()) {
            continue; // empty spawnarg attribute => invalid
        }

        // Get the index from the string's tail
        std::string indexStr = attr.getName().substr(prefix.length());

        const EntityClassAttribute& classAttr = def->getAttribute(diffPrefix + "class_" + indexStr);
        const EntityClassAttribute& argAttr = def->getAttribute(diffPrefix + "arg_" + indexStr);

        SettingPtr setting = createSetting(classAttr.getValue());
        setting->spawnArg = attr.getValue();
        setting->argument = argAttr.getValue();

        // This has been parsed from the default entityDef
        setting->isDefault = true;

        // Interpret/parse the argument string
        setting->parseAppType();
    }

    clearTreeModel();
    updateTreeModel();
}

void DifficultySettings::parseFromMapEntity(Entity* entity) {
    // Construct the prefix for the desired difficulty level
    std::string diffPrefix = "diff_" + string::to_string(_level) + "_";
    std::string prefix = diffPrefix + "change_";

    Entity::KeyValuePairs spawnargs = entity->getKeyValuePairs(prefix);

    for (Entity::KeyValuePairs::iterator i = spawnargs.begin();
         i != spawnargs.end(); ++i)
    {
        const std::string& key = i->first;
        const std::string& value = i->second;

        if (value.empty()) {
            continue; // empty spawnarg attribute => invalid
        }

        // Get the index from the string's tail
        std::string indexStr = key.substr(prefix.length());

        std::string className = entity->getKeyValue(diffPrefix + "class_" + indexStr);
        SettingPtr setting = createSetting(className);
        setting->spawnArg = value;
        setting->argument = entity->getKeyValue(diffPrefix + "arg_" + indexStr);

        // This has been parsed from the default entityDef
        setting->isDefault = false;

        // Interpret/parse the argument string
        setting->parseAppType();
    }

    clearTreeModel();
    updateTreeModel();
}

void DifficultySettings::saveToEntity(DifficultyEntity& target)
{
    // Cycle through all settings
    for (SettingsMap::iterator i = _settings.begin(); i != _settings.end(); ++i)
    {
        const SettingPtr& setting = i->second;

        if (setting->isDefault) {
            // Don't save default settings
            continue;
        }

        // Write the setting to the entity
        target.writeSetting(setting, _level);
    }
}

} // namespace difficulty
