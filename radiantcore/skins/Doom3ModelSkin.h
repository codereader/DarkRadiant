#pragma once

#include "modelskin.h"
#include "decl/DeclarationBase.h"

#include <string>
#include <memory>

namespace skins
{

/**
 * A single instance of a Doom 3 model skin. This structure stores a set of
 * maps between an existing texture and a new texture, and possibly the name of
 * the model that this skin is associated with.
 */
class Skin :
    public decl::DeclarationBase<decl::ISkin>
{
    // The list of models this skin is matching
    std::set<std::string> _matchingModels;

	// Ordered list of texture remaps (as they appear in the decl)
    std::vector<Remapping> _remaps;

public:
    Skin(const std::string& name);

    std::string getName() const;

    std::string getSkinFileName() const;

    const std::set<std::string>& getModels() override;
    const std::vector<Remapping>& getAllRemappings() override;

	// Get this skin's remap for the provided material name (if any).
    std::string getRemap(const std::string& name) override;

	// Add a remap pair to this skin
    void addRemap(const std::string& src, const std::string& dst);

    // Visit the functor with the name of each model mentioned in this skin declaration
    void foreachMatchingModel(const std::function<void(const std::string&)>& functor);

protected:
    void onBeginParsing() override;
    void parseFromTokens(parser::DefTokeniser& tokeniser) override;
};


}
