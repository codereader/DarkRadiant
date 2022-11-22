#pragma once

#include <vector>
#include <set>
#include "imodule.h"
#include <sigc++/signal.h>
#include "ideclmanager.h"

constexpr const char* const SKINS_FOLDER = "skins/";
constexpr const char* const SKIN_FILE_EXTENSION = ".skin";

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

    // Adds the given model to the set of associated models (does nothing if the model is already present)
    virtual void addModel(const std::string& model) = 0;

    // Removes the given model from the set of models (does nothing if the model is not listed)
    virtual void removeModel(const std::string& model) = 0;

    // The list of all remappings defined in this skin
    virtual const std::vector<Remapping>& getAllRemappings() = 0;

    // Adds the given remapping to this skin (doesn't check for redundancy)
    virtual void addRemapping(const Remapping& remapping) = 0;

    // Removes the remapping for the given (source) material
    virtual void removeRemapping(const std::string& material) = 0;

    // Removes all remappings from this skin
    virtual void clearRemappings() = 0;

    // Returns true if the skin has changed since it has been parsed
    virtual bool isModified() = 0;

    // Save any modifications, after this call isModified() will return false
    virtual void commitModifications() = 0;

    // Discard any modifications, reverting this to the state it had after parsing.
    // After this call isModified() will return false again.
    virtual void revertModifications() = 0;
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
     * Copies the existing material and creates a new skin with the suggested name.
     * If the suggested name is already present, a non-conflicting name will be generated and used instead.
     * Returns the new skin reference - or an empty reference if the original skin is not existing
     * or if the suggested name is empty.
     */
    virtual ISkin::Ptr copySkin(const std::string& nameOfOriginal, const std::string& nameOfCopy) = 0;

    // Renames the skin named oldName to newName, and returns true if the operation was successful. 
    // If the new name is already in use, this returns false too.
    virtual bool renameSkin(const std::string& oldName, const std::string& newName) = 0;

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
     * A skin can be modified if it has been declared in a physical file,
     * i.e. outside a PAK file.
     */
    virtual bool skinCanBeModified(const std::string& name) = 0;

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
