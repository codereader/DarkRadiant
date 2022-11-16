#include "Doom3SkinCache.h"

#include "iscenegraph.h"
#include "ideclmanager.h"
#include "module/StaticModule.h"
#include "decl/DeclarationCreator.h"

namespace skins
{

decl::ISkin::Ptr Doom3SkinCache::findSkin(const std::string& name)
{
    return std::static_pointer_cast<decl::ISkin>(
        GlobalDeclarationManager().findDeclaration(decl::Type::Skin, name)
    );
}

bool Doom3SkinCache::renameSkin(const std::string& oldName, const std::string& newName)
{
    auto result = GlobalDeclarationManager().renameDeclaration(decl::Type::Skin, oldName, newName);

    if (result)
    {
        // Look up the changed skin and mark it as modified
        auto skin = std::static_pointer_cast<Skin>(
            GlobalDeclarationManager().findDeclaration(decl::Type::Skin, newName));
        skin->setIsModified();
    }

    return result;
}

const StringList& Doom3SkinCache::getSkinsForModel(const std::string& model)
{
    static StringList _emptyList;

    std::lock_guard<std::mutex> lock(_cacheLock);

    auto existing = _modelSkins.find(model);
    return existing != _modelSkins.end() ? existing->second : _emptyList;
}

const StringList& Doom3SkinCache::getAllSkins()
{
    std::lock_guard<std::mutex> lock(_cacheLock);

    return _allSkins;
}

bool Doom3SkinCache::skinCanBeModified(const std::string& name)
{
    auto decl = findSkin(name);

    if (!decl) return false;

    const auto& fileInfo = decl->getBlockSyntax().fileInfo;
    return fileInfo.name.empty() || fileInfo.getIsPhysicalFile();
}

sigc::signal<void> Doom3SkinCache::signal_skinsReloaded()
{
	return _sigSkinsReloaded;
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
		_dependencies.insert(MODULE_DECLMANAGER);
	}

	return _dependencies;
}

void Doom3SkinCache::refresh()
{
    GlobalDeclarationManager().reloadDeclarations();
}

void Doom3SkinCache::initialiseModule(const IApplicationContext& ctx)
{
    GlobalDeclarationManager().registerDeclType("skin", std::make_shared<decl::DeclarationCreator<Skin>>(decl::Type::Skin));
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Skin, SKINS_FOLDER, SKIN_FILE_EXTENSION);

    _declsReloadedConnection = GlobalDeclarationManager().signal_DeclsReloaded(decl::Type::Skin).connect(
        sigc::mem_fun(this, &Doom3SkinCache::onSkinDeclsReloaded)
    );
}

void Doom3SkinCache::shutdownModule()
{
    _declsReloadedConnection.disconnect();

    _modelSkins.clear();
    _allSkins.clear();
}

void Doom3SkinCache::onSkinDeclsReloaded()
{
    {
        std::lock_guard<std::mutex> lock(_cacheLock);

        _modelSkins.clear();
        _allSkins.clear();

        // Re-build the lists and mappings
        GlobalDeclarationManager().foreachDeclaration(decl::Type::Skin, [&](const decl::IDeclaration::Ptr& decl)
        {
            auto skin = std::static_pointer_cast<Skin>(decl);

            _allSkins.push_back(skin->getDeclName());

            skin->foreachMatchingModel([&](const std::string& modelName)
            {
                auto& matchingSkins = _modelSkins.try_emplace(modelName).first->second;
                matchingSkins.push_back(skin->getDeclName());
            });
        });
    }

    // Run an update of the active scene, if the module is present
    if (module::GlobalModuleRegistry().moduleExists(MODULE_SCENEGRAPH))
    {
        updateModelsInScene();
    }

    signal_skinsReloaded().emit();
}

void Doom3SkinCache::updateModelsInScene()
{
    GlobalSceneGraph().foreachNode([](const scene::INodePtr& node)->bool
    {
        // Check if we have a skinnable model
        if (auto skinned = std::dynamic_pointer_cast<SkinnedModel>(node); skinned)
        {
            // Let the skinned model reload its current skin.
            skinned->skinChanged(skinned->getSkin());
        }

        return true; // traverse further
    });
}

// Module instance
module::StaticModuleRegistration<Doom3SkinCache> skinCacheModule;

} // namespace
