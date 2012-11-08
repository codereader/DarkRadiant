#pragma once

#include "isound.h"

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

namespace sound
{

/// Representation of a single sound shader.
class SoundShader : public ISoundShader
{
	// Name of the shader
	std::string _name;

	// The raw unparsed definition block
	std::string _blockContents;

    // Information we have parsed on demand
    class ParsedContents;
    mutable boost::scoped_ptr<ParsedContents> _contents;

	// The modname (ModResource implementation)
	std::string _modName;

private:
	// Parses the definition block
	void parseDefinition() const;

public:

	/// Constructor.
	SoundShader(const std::string& name,
				const std::string& blockContents,
				const std::string& modName = "base");
    ~SoundShader();

    // ISoundShader implementation
	SoundRadii getRadii() const;
	std::string getName() const { return _name; }
	SoundFileList getSoundFileList() const;
	std::string getModName() const { return _modName; }
	const std::string& getDisplayFolder() const;
};

/**
 * Shared pointer type.
 */
typedef boost::shared_ptr<SoundShader> SoundShaderPtr;

}
