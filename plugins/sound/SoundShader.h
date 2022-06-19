#pragma once

#include <memory>

#include "isound.h"
#include "ifilesystem.h"

namespace sound
{

/// Representation of a single sound shader.
class SoundShader :
	public ISoundShader
{
	// Name of the shader
	std::string _name;

	// The raw unparsed definition block
	std::string _blockContents;

    // Information we have parsed on demand
    struct ParsedContents;
    mutable std::unique_ptr<ParsedContents> _contents;

	vfs::FileInfo _fileInfo;

	// The modname (ModResource implementation)
	std::string _modName;

private:
	// Parses the definition block
	void parseDefinition() const;

public:
	using Ptr = std::shared_ptr<SoundShader>;

	/// Constructor.
	SoundShader(const std::string& name,
				const std::string& blockContents,
				const vfs::FileInfo& fileInfo,
				const std::string& modName = "base");

    virtual ~SoundShader();

    // ISoundShader implementation
	SoundRadii getRadii() const override;
	const std::string& getDeclName() const override { return _name; }
	decl::Type getDeclType() const override { return decl::Type::SoundShader; }
	SoundFileList getSoundFileList() const override;
	std::string getModName() const override { return _modName; }
	const std::string& getDisplayFolder() const override;
	std::string getShaderFilePath() const override;
	std::string getDefinition() const override;
};

}
