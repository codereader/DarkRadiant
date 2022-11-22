#include "Doom3SkinCache.h"

#include "iscenegraph.h"
#include "ifiletypes.h"
#include "ideclmanager.h"
#include "module/StaticModule.h"
#include "decl/DeclarationCreator.h"
#include "decl/DeclLib.h"

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

decl::ISkin::Ptr Doom3SkinCache::copySkin(const std::string& nameOfOriginal, const std::string& nameOfCopy)
{
    if (nameOfCopy.empty())
    {
        rWarning() << "Cannot copy, the new name must not be empty" << std::endl;
        return {};
    }

    auto candidate = decl::generateNonConflictingName(decl::Type::Skin, nameOfCopy);

    auto existing = GlobalDeclarationManager().findDeclaration(decl::Type::Skin, nameOfOriginal);

    if (!existing)
    {
        rWarning() << "Cannot copy non-existent skin " << nameOfOriginal << std::endl;
        return {};
    }

    auto copiedSkin = std::static_pointer_cast<Skin>(
        GlobalDeclarationManager().findOrCreateDeclaration(decl::Type::Skin, candidate));

    // Replace the syntax block of the target with the one of the original
    auto syntax = existing->getBlockSyntax();
    syntax.name = nameOfCopy;
    syntax.fileInfo = vfs::FileInfo{ "", "", vfs::Visibility::HIDDEN };

    copiedSkin->setBlockSyntax(syntax);
    copiedSkin->setIsModified();

    return copiedSkin;
}

const StringList& Doom3SkinCache::getSkinsForModel(const std::string& model)
{
    static StringList _emptyList;

    std::lock_guard<std::mutex> lock(_cacheLock);

    ensureCacheIsUpdated();

    auto existing = _modelSkins.find(model);
    return existing != _modelSkins.end() ? existing->second : _emptyList;
}

const StringList& Doom3SkinCache::getAllSkins()
{
    std::lock_guard<std::mutex> lock(_cacheLock);

    ensureCacheIsUpdated();

    return _allSkins;
}

bool Doom3SkinCache::skinCanBeModified(const std::string& name)
{
    auto decl = findSkin(name);

    if (!decl) return false;

    const auto& fileInfo = decl->getBlockSyntax().fileInfo;
    return fileInfo.name.empty() || fileInfo.getIsPhysicalFile();
}

void Doom3SkinCache::ensureCacheIsUpdated()
{
    if (_skinsPendingReparse.empty()) return;

    for (const auto& name : _skinsPendingReparse)
    {
        handleSkinRemoval(name);

        // Only add the skin if it's still existing
        if (findSkin(name))
        {
            handleSkinAddition(name);
        }
    }

    _skinsPendingReparse.clear();
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
		_dependencies.insert(MODULE_FILETYPES);
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
    GlobalFiletypes().registerPattern("skin", FileTypePattern(_("Skin File"), "skin", "*.skin"));

    _declsReloadedConnection = GlobalDeclarationManager().signal_DeclsReloaded(decl::Type::Skin).connect(
        sigc::mem_fun(this, &Doom3SkinCache::onSkinDeclsReloaded)
    );

    _declCreatedConnection = GlobalDeclarationManager().signal_DeclCreated().connect(
        sigc::mem_fun(this, &Doom3SkinCache::onSkinDeclCreated)
    );
    _declRemovedConnection = GlobalDeclarationManager().signal_DeclRemoved().connect(
        sigc::mem_fun(this, &Doom3SkinCache::onSkinDeclRemoved)
    );
    _declRenamedConnection = GlobalDeclarationManager().signal_DeclRenamed().connect(
        sigc::mem_fun(this, &Doom3SkinCache::onSkinDeclRenamed)
    );
}

void Doom3SkinCache::shutdownModule()
{
    unsubscribeFromAllSkins();

    _declCreatedConnection.disconnect();
    _declRenamedConnection.disconnect();
    _declRemovedConnection.disconnect();
    _declsReloadedConnection.disconnect();

    _modelSkins.clear();
    _allSkins.clear();
    _skinsPendingReparse.clear();
}

void Doom3SkinCache::subscribeToSkin(const decl::ISkin::Ptr& skin)
{
    _declChangedConnections[skin->getDeclName()] = skin->signal_DeclarationChanged().connect(
        [skinPtr = skin.get(), this] { onSkinDeclChanged(*skinPtr); }
    );
}

void Doom3SkinCache::handleSkinAddition(const std::string& name)
{
    // Add the skin to the cached lists
    _allSkins.push_back(name);

    auto skin = findSkin(name);
    if (!skin) return;

    for (const auto& modelName : skin->getModels())
    {
        auto& matchingSkins = _modelSkins.try_emplace(modelName).first->second;
        matchingSkins.push_back(skin->getDeclName());
    }

    subscribeToSkin(skin);
}

void Doom3SkinCache::handleSkinRemoval(const std::string& name)
{
    _declChangedConnections.erase(name);

    // Remove the skin from the cached lists
    auto allSkinIt = std::find(_allSkins.begin(), _allSkins.end(), name);
    if (allSkinIt != _allSkins.end())
    {
        _allSkins.erase(allSkinIt);
    }

    for (auto& [_, matchingSkins] : _modelSkins)
    {
        auto found = std::find(matchingSkins.begin(), matchingSkins.end(), name);
        if (found != matchingSkins.end())
        {
            matchingSkins.erase(found);
        }
    }
}

void Doom3SkinCache::onSkinDeclCreated(decl::Type type, const std::string& name)
{
    if (type != decl::Type::Skin) return;

    std::lock_guard<std::mutex> lock(_cacheLock);
    handleSkinAddition(name);
}

void Doom3SkinCache::onSkinDeclRemoved(decl::Type type, const std::string& name)
{
    if (type != decl::Type::Skin) return;

    std::lock_guard<std::mutex> lock(_cacheLock);
    handleSkinRemoval(name);
    _skinsPendingReparse.erase(name);
}

void Doom3SkinCache::onSkinDeclRenamed(decl::Type type, const std::string& oldName, const std::string& newName)
{
    if (type != decl::Type::Skin) return;

    std::lock_guard<std::mutex> lock(_cacheLock);
    handleSkinRemoval(oldName);
    handleSkinAddition(newName);
}

void Doom3SkinCache::unsubscribeFromAllSkins()
{
    for (auto& [_, conn] : _declChangedConnections)
    {
        conn.disconnect();
    }

    _declChangedConnections.clear();
}

void Doom3SkinCache::onSkinDeclChanged(decl::ISkin& skin)
{
    std::lock_guard<std::mutex> lock(_cacheLock);

    // Add it to the pile, it will be processed once we need to access the cached lists
    _skinsPendingReparse.insert(skin.getDeclName());
}

void Doom3SkinCache::onSkinDeclsReloaded()
{
    {
        std::lock_guard<std::mutex> lock(_cacheLock);

        unsubscribeFromAllSkins();
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

            subscribeToSkin(skin);
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
