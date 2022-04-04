#include "Doom3SkinCache.h"

#include "itextstream.h"
#include "ifilesystem.h"
#include "iarchive.h"
#include "module/StaticModule.h"

#include <iostream>

namespace skins
{

Doom3SkinCache::Doom3SkinCache() :
    _nullSkin("")
{
    _defLoader.signal_finished().connect(
        [this]() { _sigSkinsReloaded.emit(); }
    );
}

ModelSkin& Doom3SkinCache::capture(const std::string& name)
{
    ensureDefsLoaded();

    auto i = _namedSkins.find(name);

	return i != _namedSkins.end() ? *(i->second) : _nullSkin;
}

const StringList& Doom3SkinCache::getSkinsForModel(const std::string& model)
{
    ensureDefsLoaded();
    return _modelSkins[model];
}

const StringList& Doom3SkinCache::getAllSkins()
{
    ensureDefsLoaded();
    return _allSkins;
}

void Doom3SkinCache::addNamedSkin(const ModelSkinPtr& modelSkin)
{
    _namedSkins[modelSkin->getName()] = modelSkin;
    _allSkins.emplace_back(modelSkin->getName());
}

void Doom3SkinCache::removeSkin(const std::string& name)
{
    _namedSkins.erase(name);
    _allSkins.erase(std::find(_allSkins.begin(), _allSkins.end(), name));
}

sigc::signal<void> Doom3SkinCache::signal_skinsReloaded()
{
	return _sigSkinsReloaded;
}

// Realise the skin cache
void Doom3SkinCache::ensureDefsLoaded()
{
    if (_allSkins.empty())
    {
        // Get the result of the loader and move the contents
        auto result = _defLoader.get();

        _allSkins = std::move(result->allSkins);
        _modelSkins = std::move(result->modelSkins);
        _namedSkins = std::move(result->namedSkins);
    }
}

const std::string& Doom3SkinCache::getName() const
{
	static std::string _name(MODULE_MODELSKINCACHE);
	return _name;
}

const StringSet& Doom3SkinCache::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
    {
		_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
	}

	return _dependencies;
}

void Doom3SkinCache::refresh()
{
	_modelSkins.clear();
	_namedSkins.clear();
	_allSkins.clear();

    // Reset loader and launch a new thread
    _defLoader.reset();
    _defLoader.start();
}

void Doom3SkinCache::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called" << std::endl;

    // Load the skins in a new thread
    refresh();
}

// Module instance
module::StaticModuleRegistration<Doom3SkinCache> skinCacheModule;

} // namespace
