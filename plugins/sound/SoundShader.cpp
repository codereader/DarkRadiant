#include "SoundShader.h"

#include "parser/DefTokeniser.h"
#include "string/string.h"
#include <boost/algorithm/string/predicate.hpp>

namespace sound {

void SoundShader::parseDefinition() {
	// Flag as parsed from now on
	_parsed = true;

	// Get a new tokeniser and parse the block
	parser::BasicDefTokeniser<std::string> tok(_blockContents);

	while (tok.hasMoreTokens()) {
		// Get the next token
		std::string nextToken = tok.nextToken();

		// Watch out for sound file definitions and min/max radii
		if (boost::algorithm::starts_with(nextToken, "sound/")) {
			// Add this to the list
			_soundFiles.push_back(nextToken);
		}
		else if (nextToken == "minDistance") {
			// Set the radius and convert to metres
			_soundRadii.setMin(strToFloat(tok.nextToken()), true);
		}
		else if (nextToken == "maxDistance") {
			// Set the radius and convert to metres
			_soundRadii.setMax(strToFloat(tok.nextToken()), true);
		}
	}
}

} // namespace sound
