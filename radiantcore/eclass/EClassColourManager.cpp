#include "EClassColourManager.h"

#include "itextstream.h"
#include "module/StaticModule.h"

namespace eclass
{

void EClassColourManager::addOverrideColour(const std::string& eclass, const Vector3& colour)
{
    _overrides[eclass] = colour;
}

void EClassColourManager::applyColours(const IEntityClassPtr& eclass)
{
    assert(eclass);

    auto foundOverride = _overrides.find(eclass->getName());

    if (foundOverride != _overrides.end())
    {
        rDebug() << "Applying colour " << foundOverride->second << " to eclass " << eclass->getName() << std::endl;

        eclass->setColour(foundOverride->second);
    }
}

void EClassColourManager::foreachOverrideColour(
    const std::function<void(const std::string&, const Vector3&)>& functor)
{
    for (const auto& pair : _overrides)
    {
        functor(pair.first, pair.second);
    }
}

void EClassColourManager::removeOverrideColour(const std::string& eclass)
{
    _overrides.erase(eclass);
}

const std::string& EClassColourManager::getName() const
{
    static std::string _name(MODULE_ECLASS_COLOUR_MANAGER);
    return _name;
}

const StringSet& EClassColourManager::getDependencies() const
{
    static StringSet _dependencies;
    return _dependencies;
}

void EClassColourManager::initialiseModule(const IApplicationContext& ctx)
{
    rMessage() << getName() << "::initialiseModule called." << std::endl;
}

module::StaticModule<EClassColourManager> eclassColourManagerModule;

}
