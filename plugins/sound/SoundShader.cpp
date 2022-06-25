#include "SoundShader.h"

#include "parser/DefTokeniser.h"
#include "string/convert.h"
#include <algorithm>
#include "string/predicate.h"

namespace sound
{

struct SoundShader::ParsedContents
{
    // List of sound files
	SoundFileList soundFiles;

	// min and max radii of the shader
	SoundRadii soundRadii;

	// display folder including slashes for sorting the sounds in the sound chooser window
	std::string displayFolder;
};

SoundShader::SoundShader(const std::string& name)
:	_name(name),
    _parseStamp(0)
{ }

// Destructor must be defined with ParsedContents definition visible, otherwise
// the scoped_ptr destructor will not compile
SoundShader::~SoundShader()
{ }

void SoundShader::parseDefinition() const
{
	_contents = std::make_unique<ParsedContents>();

	// Get a new tokeniser and parse the block
	parser::BasicDefTokeniser<std::string> tok(_declBlock.contents);

	while (tok.hasMoreTokens())
    {
		// Get the next token
		std::string nextToken = tok.nextToken();

		// Watch out for sound file definitions
        // Check if the token starts with soundSLASH or soundBACKSLASH
		if (string::starts_with(nextToken, "sound") && 
            nextToken.length() > 5 && (nextToken[5] == '\\' || nextToken[5] == '/'))
        {
			// Add this to the list, replace backslashes with forward ones
            std::replace(nextToken.begin(), nextToken.end(), '\\', '/');

			_contents->soundFiles.push_back(nextToken);
		}
		else if (nextToken == "minDistance")
        {
			// Set the radius and convert to metres
			_contents->soundRadii.setMin(string::convert<float>(tok.nextToken()), true);
		}
		else if (nextToken == "maxDistance")
        {
			// Set the radius and convert to metres
			_contents->soundRadii.setMax(string::convert<float>(tok.nextToken()), true);
		}
		else if (nextToken == "editor_displayFolder")
        {
			// Set the display folder
			_contents->displayFolder = tok.nextToken();
		}
	}
}

SoundRadii SoundShader::getRadii() const
{
    if (!_contents) parseDefinition();
    return _contents->soundRadii;
}

SoundFileList SoundShader::getSoundFileList() const
{
    if (!_contents) parseDefinition();
    return _contents->soundFiles;
}

const std::string& SoundShader::getDisplayFolder() const
{
    if (!_contents) parseDefinition();
    return _contents->displayFolder;
}

std::string SoundShader::getShaderFilePath() const
{
	return _fileInfo.fullPath();
}

std::string SoundShader::getDefinition() const
{
	return _declBlock.contents;
}

const decl::DeclarationBlockSyntax& SoundShader::getBlockSyntax() const
{
    return _declBlock;
}

void SoundShader::setBlockSyntax(const decl::DeclarationBlockSyntax& block)
{
    _declBlock = block;
    _fileInfo = block.fileInfo;
    _modName = block.getModName();

    // Reset any contents, we reparse as soon as any property is accessed
    _contents.reset();
}

std::size_t SoundShader::getParseStamp() const
{
    return _parseStamp;
}

void SoundShader::setParseStamp(std::size_t parseStamp)
{
    _parseStamp = parseStamp;
}

} // namespace sound
