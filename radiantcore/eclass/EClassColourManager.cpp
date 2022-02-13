#include "EClassColourManager.h"

#include "itextstream.h"
#include "module/StaticModule.h"

namespace eclass
{

void EClassColourManager::addOverrideColour(const std::string& eclass, const Vector4& colour)
{
    _overrides[eclass] = colour;
    _overrideChangedSignal.emit(eclass, false); // false ==> colour added
}

bool EClassColourManager::applyColours(IEntityClass& eclass)
{
    auto foundOverride = _overrides.find(eclass.getName());
    if (foundOverride != _overrides.end())
    {
        eclass.setColour(foundOverride->second);
        return true;
    }
    return false;
}

void EClassColourManager::foreachOverrideColour(
    const std::function<void(const std::string&, const Vector4&)>& functor)
{
    for (const auto& pair : _overrides)
    {
        functor(pair.first, pair.second);
    }
}

void EClassColourManager::removeOverrideColour(const std::string& eclass)
{
    _overrides.erase(eclass);
    _overrideChangedSignal.emit(eclass, true); // true ==> colour removed
}

void EClassColourManager::clearOverrideColours()
{
    for (auto i = _overrides.begin(); i != _overrides.end(); ++i)
    {
        // Copy the eclass name to a local
        auto eclass = i->first;

        // Delete from map
        _overrides.erase(i++);

        // Fire signal, this might call applyColours which will
        // find the colour has have been removed
        _overrideChangedSignal.emit(eclass, true); // true ==> colour removed
    }
}

sigc::signal<void, const std::string&, bool>& EClassColourManager::sig_overrideColourChanged()
{
    return _overrideChangedSignal;
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

module::StaticModuleRegistration<EClassColourManager> eclassColourManagerModule;

}
