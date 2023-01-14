#pragma once

#include "modelskin.h"

#include <string>
#include <memory>

#include "decl/EditableDeclaration.h"

namespace skins
{

/**
 * A single instance of a Doom 3 model skin. This structure stores a set of
 * maps between an existing texture and a new texture, and possibly the name of
 * the model that this skin is associated with.
 */
class Skin :
    public decl::EditableDeclaration<decl::ISkin>
{
    struct SkinData
    {
        using Ptr = std::shared_ptr<SkinData>;

        // The list of models this skin is matching
        std::set<std::string> matchingModels;

	    // Ordered list of texture remaps (as they appear in the decl)
        std::vector<Remapping> remaps;
    };

    // The unchanged skin data
    SkinData::Ptr _original;

    // The current skin data (might be modified)
    SkinData::Ptr _current;

public:
    Skin(const std::string& name);

    std::string getName() const;

    std::string getSkinFileName() const;

    const std::set<std::string>& getModels() override;
    void addModel(const std::string& model) override;
    void removeModel(const std::string& model) override;

    const std::vector<Remapping>& getAllRemappings() override;
    void addRemapping(const Remapping& remapping) override;
    void removeRemapping(const std::string& material) override;
    void clearRemappings() override;

	// Get this skin's remap for the provided material name (if any).
    std::string getRemap(const std::string& name) override;

	// Add a remap pair to this skin
    void addRemap(const std::string& src, const std::string& dst);

    // Visit the functor with the name of each model mentioned in this skin declaration
    void foreachMatchingModel(const std::function<void(const std::string&)>& functor);

    bool isModified() override;
    void setIsModified();
    void commitModifications() override;
    void revertModifications() override;

protected:
    void onBeginParsing() override;
    void parseFromTokens(parser::DefTokeniser& tokeniser) override;
    std::string generateSyntax() override;

private:
    void ensureSkinDataBackup();
};


}
