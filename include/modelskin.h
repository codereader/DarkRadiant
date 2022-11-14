#pragma once

#include <vector>
#include <set>
#include "imodule.h"
#include <sigc++/signal.h>
#include "ideclmanager.h"

namespace decl
{

class ISkin :
    public IDeclaration
{
public:
    using Ptr = std::shared_ptr<ISkin>;

    ~ISkin() override {}

    struct Remapping
    {
        // The original material name (can also be a wildcard "*")
        std::string Original;

        // The replacement material name
        std::string Replacement;
    };

    /**
     * Get the mapped texture for the given query texture, using the mappings
     * in this skin. If there is no mapping for the given texture, return an
     * empty string.
     */
    virtual std::string getRemap(const std::string& name) = 0;

    // Returns the list of models this skin applies to
    virtual const std::set<std::string>& getModels() = 0;

    // The list of all remappings defined in this skin
    virtual const std::vector<Remapping>& getAllRemappings() = 0;
};

} // namespace

class SkinnedModel
{
public:
    // destructor
	virtual ~SkinnedModel() {}

	// greebo: Updates the model's surface remaps. Pass the new skin name (can be empty).
	virtual void skinChanged(const std::string& newSkinName) = 0;

	// Returns the name of the currently active skin
	virtual std::string getSkin() const = 0;
};
typedef std::shared_ptr<SkinnedModel> SkinnedModelPtr;

// Model skinlist typedef
typedef std::vector<std::string> StringList;

constexpr const char* const MODULE_MODELSKINCACHE("ModelSkinCache");

namespace decl
{

/**
 * Interface class for the skin manager.
 */
class IModelSkinCache :
	public RegisterableModule
{
public:
	/**
	 * Lookup a specific named skin and return the corresponding skin object.
	 * Returns an empty reference if no declaration is matching the name.
	 */
	virtual ISkin::Ptr findSkin(const std::string& name) = 0;

	/**
	 * Return the skins associated with the given model.
	 *
	 * @param
	 * The full pathname of the model, as given by the "model" key in the skin
	 * definition.
	 *
	 * @returns
	 * A vector of strings, each identifying the name of a skin which is
	 * associated with the given model. The vector may be empty as a model does
	 * not require any associated skins.
	 */
	virtual const StringList& getSkinsForModel(const std::string& model) = 0;

	/**
	 * Return the complete list of available skins.
	 */
	virtual const StringList& getAllSkins() = 0;

	/**
	 * greebo: Reloads all skins from the definition files.
	 */
	virtual void refresh() = 0;

	/// Signal emitted after skins are reloaded
	virtual sigc::signal<void> signal_skinsReloaded() = 0;
};

} // namespace

inline decl::IModelSkinCache& GlobalModelSkinCache()
{
	static module::InstanceReference<decl::IModelSkinCache> _reference(MODULE_MODELSKINCACHE);
	return _reference;
}
