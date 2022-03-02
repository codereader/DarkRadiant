#pragma once

#include "Doom3ModelSkin.h"

#include "imodule.h"
#include "modelskin.h"
#include "parser/DefTokeniser.h"

#include <map>
#include <string>
#include <vector>
#include "SkinDeclParser.h"

namespace skins
{

/**
 * Implementation of ModelSkinCache interface for Doom 3 skin management.
 */
class Doom3SkinCache :
	public ModelSkinCache
{
	// Table of named skin objects
    std::map<std::string, ModelSkinPtr> _namedSkins;

	// List of all skins
	std::vector<std::string> _allSkins;

	// Map between model paths and a vector of names of the associated skins,
	// which are contained in the main NamedSkinMap.
    std::map<std::string, std::vector<std::string>> _modelSkins;

    // Helper which will load the skin defs in a separate thread
    SkinDeclParser _defLoader;

	// Empty Doom3ModelSkin to return if a named skin is not found
	Doom3ModelSkin _nullSkin;

	sigc::signal<void> _sigSkinsReloaded;

public:
    Doom3SkinCache();

	/* Return a specific named skin. If the named skin cannot be found, return
	 * the empty (null) skin with no remaps.
	 */
    ModelSkin& capture(const std::string& name) override;

	/* Get the vector of skin names corresponding to the given model.
	 */
    const StringList& getSkinsForModel(const std::string& model) override;

	/* Return a complete list of skins.
	 */
    const StringList& getAllSkins() override;

    void addNamedSkin(const ModelSkinPtr& modelSkin) override;
    void removeSkin(const std::string& name) override;

	/**
	 * greebo: Clears and reloads all skins.
	 */
	void refresh() override;

	// Public events
	sigc::signal<void> signal_skinsReloaded() override;

	// RegisterableModule implementation
	const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;

private:
    // Load and parse the skin files, populating internal data structures.
    // Function may be called more than once, will do nothing if already
    // realised.
    void ensureDefsLoaded();

    // Iterates over each skin file in the VFS skins/ folder
    void loadSkinFiles();

    // Parse an individual skin declaration and add return the skin object
    Doom3ModelSkinPtr parseSkin(parser::DefTokeniser& tokeniser);

    /* Parse the provided istream as a .skin file, and add all skins found within
    * to the internal data structures.
    *
    * @filename: This is for informational purposes only (error message display).
    */
    void parseFile(std::istream& contents, const std::string& filename);
};

} // namespace skins
