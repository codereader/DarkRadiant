#pragma once

#include <memory>

#include "isound.h"
#include "ifilesystem.h"
#include "decl/DeclarationBase.h"

namespace sound
{

/// Representation of a single sound shader.
class SoundShader final :
	public decl::DeclarationBase<ISoundShader>
{
    // Information we have parsed on demand
    struct ParsedContents;
    mutable std::unique_ptr<ParsedContents> _contents;

public:
	using Ptr = std::shared_ptr<SoundShader>;

	SoundShader(const std::string& name);

    ~SoundShader();

    // ISoundShader implementation
	SoundRadii getRadii() override;
	SoundFileList getSoundFileList() override;
	const std::string& getDisplayFolder() override;

protected:
    void onBeginParsing() override;
    void parseFromTokens(parser::DefTokeniser& tokeniser) override;
};

}
