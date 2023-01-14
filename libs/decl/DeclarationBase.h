#pragma once

#include "ideclmanager.h"
#include "parser/DefTokeniser.h"

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
 *
 * Declaration types that can be modified through its public API should inherit
 * and implement the EditableDeclaration<> template.
 */
template<typename DeclarationInterface>
class DeclarationBase :
    public DeclarationInterface
{
private:
    static_assert(std::is_base_of_v<IDeclaration, DeclarationInterface>, 
        "DeclarationInterface type must inherit from IDeclaration");

    std::string _name;
    std::string _originalName; // the original name as parsed from the file
    decl::Type _type;

    std::size_t _parseStamp;

    // The raw unparsed definition block
    DeclarationBlockSyntax _declBlock;

    bool _parsed;
    std::string _parseErrors;

    sigc::signal<void> _changedSignal;

protected:
    DeclarationBase(decl::Type type, const std::string& name) :
        _name(name),
        _originalName(name),
        _type(type),
        _parseStamp(0),
        _parsed(false)
    {}

    DeclarationBase(const DeclarationBase<DeclarationInterface>& other) = default;

public:
    const std::string& getDeclName() const final
    {
        return _name;
    }
    
    void setDeclName(const std::string& newName) override
    {
        _name = newName;
        _declBlock.name = newName;
    }

    const std::string& getOriginalDeclName() const final
    {
        return _originalName;
    }

    void setOriginalDeclName(const std::string& newName) override
    {
        _originalName = newName;
    }

    decl::Type getDeclType() const final
    {
        return _type;
    }

    const DeclarationBlockSyntax& getBlockSyntax() override
    {
        return _declBlock;
    }

    void setBlockSyntax(const DeclarationBlockSyntax& block) final
    {
        _declBlock = block;

        // Reset the parsed flag and notify the subclasses
        _parsed = false;

        onSyntaxBlockAssigned(_declBlock);

        _changedSignal.emit();
    }

    std::string getModName() const final
    {
        return _declBlock.getModName();
    }

    std::string getDeclFilePath() const final
    {
        return _declBlock.fileInfo.fullPath();
    }

    void setFileInfo(const vfs::FileInfo& fileInfo) override
    {
        _declBlock.fileInfo = fileInfo;
    }

    std::size_t getParseStamp() const final
    {
        return _parseStamp;
    }

    void setParseStamp(std::size_t parseStamp) final
    {
        _parseStamp = parseStamp;
    }

    sigc::signal<void>& signal_DeclarationChanged() final
    {
        return _changedSignal;
    }

    const std::string& getParseErrors()
    {
        ensureParsed();
        return _parseErrors;
    }

protected:
    // Defines the whitespace characters used by the DefTokeniser to separate tokens
    virtual const char* getWhitespaceDelimiters() const
    {
        return parser::WHITESPACE;
    }

    // Defines the characters separating tokens and are considered tokens themselves
    virtual const char* getKeptDelimiters() const
    {
        return "{}()";
    }

    // Subclasses should call this to ensure the attached syntax block has been processed.
    // In case the block needs parsing, the parseFromTokens() method will be invoked,
    // followed by an onParseFinished() call (the latter of which is invoked regardless
    // of any parse exceptions that might have been occurring).
    void ensureParsed()
    {
        if (_parsed) return;

        // Set the flag to true before parsing, to avoid infinite loops
        _parsed = true;
        _parseErrors.clear();

        onBeginParsing();

        try
        {
            // Set up a tokeniser to let the subclass implementation parse the contents
            parser::BasicDefTokeniser<std::string> tokeniser(getBlockSyntax().contents,
                getWhitespaceDelimiters(), getKeptDelimiters());
            parseFromTokens(tokeniser);
        }
        catch (const parser::ParseException& ex)
        {
            _parseErrors = ex.what();

            rError() << "[DeclParser]: Error parsing " << getTypeName(getDeclType()) << " " << getDeclName()
                << ": " << ex.what() << std::endl;
        }

        onParsingFinished();
    }

    // Optional callback to be overridden by subclasses.
    // Will always be called before parseFromTokens().
    virtual void onBeginParsing()
    {}

    // To be implemented by subclasses, this method will parse the contents of the decl block
    // and extract the relevant information. After parseFromTokens() a call to onParseFinished()
    // will follow, unconditionally.
    virtual void parseFromTokens(parser::DefTokeniser& tokeniser) = 0;

    // Optional callback to be overridden by subclasses.
    // Will always be called after parseFromTokens() has completed, even in case of parse errors.
    virtual void onParsingFinished()
    {}

    // Invoked after a new syntax block has been assigned through setBlockSyntax()
    // Allows subclasses to either reparse immediately or schedule a later parse
    virtual void onSyntaxBlockAssigned(const DeclarationBlockSyntax& block)
    {}

    // Updates the contents member of the attached syntax block without
    // firing any update signals. This is meant to be used internally by EditableDeclaration.
    // Assigning the block contents will not change the "parsed" status, the method
    // parseFromTokens() will not forced to be called afterwards.
    void assignSyntaxBlockContents(const std::string& newSyntax)
    {
        _declBlock.contents = newSyntax;
    }
};

}
