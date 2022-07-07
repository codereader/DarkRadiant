#pragma once

#include <memory>

#include "isound.h"
#include "ifilesystem.h"
#include "DeclarationBase.h"

namespace sound
{

/// Representation of a single sound shader.
class SoundShader final :
	public decl::DeclarationBase<ISoundShader>
{
	// Name of the shader
	std::string _name;

    // Information we have parsed on demand
    struct ParsedContents;
    mutable std::unique_ptr<ParsedContents> _contents;

private:
	// Parses the definition block
	void parseDefinition() const;

public:
	using Ptr = std::shared_ptr<SoundShader>;

	SoundShader(const std::string& name);

    ~SoundShader();

    // ISoundShader implementation
	SoundRadii getRadii() const override;
	const std::string& getDeclName() const override { return _name; }
	decl::Type getDeclType() const override { return decl::Type::SoundShader; }
	SoundFileList getSoundFileList() const override;
	std::string getModName() const override { return getBlockSyntax().getModName(); }
	const std::string& getDisplayFolder() const override;
	std::string getShaderFilePath() const override;
	std::string getDefinition() const override;

protected:
    void onSyntaxBlockAssigned(const decl::DeclarationBlockSyntax& block) override;
};

}
