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
:	DeclarationBase<ISoundShader>(decl::Type::SoundShader, name)
{}

// Destructor must be defined with ParsedContents definition visible, otherwise
// the scoped_ptr destructor will not compile
SoundShader::~SoundShader()
{ }

void SoundShader::onBeginParsing()
{
    _contents = std::make_unique<ParsedContents>();
}

void SoundShader::parseFromTokens(parser::DefTokeniser& tokeniser)
{
	while (tokeniser.hasMoreTokens())
    {
		// Get the next token
		auto nextToken = tokeniser.nextToken();

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
			_contents->soundRadii.setMin(string::convert<float>(tokeniser.nextToken()), true);
		}
		else if (nextToken == "maxDistance")
        {
			// Set the radius and convert to metres
			_contents->soundRadii.setMax(string::convert<float>(tokeniser.nextToken()), true);
		}
		else if (nextToken == "editor_displayFolder")
        {
			// Set the display folder
			_contents->displayFolder = tokeniser.nextToken();
		}
	}
}

SoundRadii SoundShader::getRadii()
{
    ensureParsed();
    return _contents->soundRadii;
}

SoundFileList SoundShader::getSoundFileList()
{
    ensureParsed();
    return _contents->soundFiles;
}

const std::string& SoundShader::getDisplayFolder()
{
    ensureParsed();
    return _contents->displayFolder;
}

} // namespace
