#pragma once

#include "DeclarationBase.h"

namespace decl
{

/**
 * Declaration class used by all types that can be changed after parsing
 * through their public API (e.g. Particles or Materials).
 *
 * Implementing classes need to invoke onParsedContentsChanged() on any
 * change of its contents.The attached syntax block will be marked as
 * outdated and calls to getSyntaxBlock() will trigger a refresh
 * of the syntax block through the generateSyntaxBlock() override.
 *
 * Note that calling setSyntaxBlock() will clear the invalid status,
 * the subsequent parseFromTokens() will very likely overwrite all changes.
 * It's up to the client code to avoid these situations.
 * On a related note, subclasses need to call ensureParsed() before altering
 * its members, to avoid parseFromTokens() from undoing the changes.
 *
 * Example implementation of a public setter method:
 *
 * void YourDeclaration::setFloatValue(float value)
 * {
 *     ensureParsed();
 *     _value = value;
 *     onParsedContentsChanged();
 * }
 */
template<typename DeclarationInterface>
class EditableDeclaration :
    public DeclarationBase<DeclarationInterface>
{
private:
    // Is set to true once the contents of the declaration
    // have been changed by the subclass (see onParsedContentsChanged).
    // The assigned syntax block's contents are no longer up to date
    // and will be regenerated before a call to getSyntaxBlock() returns.
    bool _syntaxBlockInvalidated;

protected:
    EditableDeclaration(decl::Type type, const std::string& name) :
        DeclarationBase<DeclarationInterface>(type, name),
        _syntaxBlockInvalidated(false)
    {}

    EditableDeclaration(const EditableDeclaration<DeclarationInterface>& other) = default;

public:
    const DeclarationBlockSyntax& getBlockSyntax() override
    {
        // In case the contents have been invalidated, acquire the new source text
        if (_syntaxBlockInvalidated)
        {
            _syntaxBlockInvalidated = false;
            DeclarationBase<DeclarationInterface>::assignSyntaxBlockContents(generateSyntax());
        }

        return DeclarationBase<DeclarationInterface>::getBlockSyntax();
    }

protected:
    void onSyntaxBlockAssigned(const DeclarationBlockSyntax& block) override
    {
        // Assigning a new syntax block overrides any previous edits to the decl
        // Otherwise the block contents would immediately get overwritten again
        // in getBlockSyntax() by invoking generateSyntax()
        _syntaxBlockInvalidated = false;

        DeclarationBase<DeclarationInterface>::onSyntaxBlockAssigned(block);
    }

    // Needs to be called by subclasses when members change and the
    // syntax block (that has been used to parse them) is no longer valid
    // The next time client code will acquire the syntax block, the subclass
    // will be asked to provide it through the virtual generateSyntaxBlock()
    void onParsedContentsChanged()
    {
        _syntaxBlockInvalidated = true;

        // Emit the changed signal of the base
        DeclarationBase<DeclarationInterface>::signal_DeclarationChanged().emit();
    }

    // Should generate a new syntax block based on the current state of the instance.
    // Will be invoked by the EditableDeclaration base class when clients request
    // the syntax using getSyntaxBlock() and the syntax has been invalidated by
    // onParsedContentsChanged().
    virtual std::string generateSyntax() = 0;
};

}
