#pragma once

#include "ideclmanager.h"

namespace decl
{

/**
 * Base declaration implementation shared by all decls supported by DarkRadiant.
 *
 * To avoid subclasses having to inherit the IDeclaration interface themselves
 * (and to circumvent the diamond pattern and virtual inheritance), subclasses
 * should pass the desired interface as template argument:
 * 
 * class SoundShader : public DeclarationBase<ISoundShader>
 * {}
 */
template<typename DeclarationInterface>
class DeclarationBase :
    public DeclarationInterface
{
private:
    static_assert(std::is_base_of_v<IDeclaration, DeclarationInterface>, 
        "DeclarationInterface type must inherit from IDeclaration");

    std::size_t _parseStamp;

    // The raw unparsed definition block
    DeclarationBlockSyntax _declBlock;

protected:
    DeclarationBase() :
        _parseStamp(0)
    {}

public:
    const DeclarationBlockSyntax& getBlockSyntax() const override
    {
        return _declBlock;
    }

    void setBlockSyntax(const DeclarationBlockSyntax& block) final
    {
        _declBlock = block;
        onSyntaxBlockAssigned(_declBlock);
    }

    std::size_t getParseStamp() const final
    {
        return _parseStamp;
    }

    void setParseStamp(std::size_t parseStamp) final
    {
        _parseStamp = parseStamp;
    }

protected:
    // Invoked after a new syntax block has been assigned
    // Allows subclasses to either reparse immediately or schedule a later parse
    virtual void onSyntaxBlockAssigned(const DeclarationBlockSyntax& block)
    {}
};

}
