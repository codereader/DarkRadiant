#include "Doom3AasFileLoader.h"

#include "parser/DefTokeniser.h"
#include "string/convert.h"

namespace map
{

namespace
{
    const float DEWM3_AAS_VERSION = 1.07f;
}

const std::string& Doom3AasFileLoader::getAasFormatName() const
{
    static std::string _name = "Doom 3 AAS";
	return _name;
}

const std::string& Doom3AasFileLoader::getGameType() const
{
    static std::string _gameType = "doom3";
	return _gameType;
}

bool Doom3AasFileLoader::canLoad(std::istream& stream) const
{
    // Instantiate a tokeniser to read the first few tokens
	parser::BasicDefTokeniser<std::istream> tok(stream);

	try
	{
		// Require a "Version" token
        tok.assertNextToken("DewmAAS");

		// Require specific version, return true on success 
		return (string::convert<float>(tok.nextToken()) == DEWM3_AAS_VERSION);
	}
	catch (parser::ParseException&)
	{}
	catch (boost::bad_lexical_cast&)
	{}

	return false;
}

IAasFilePtr Doom3AasFileLoader::loadFromStream(std::istream& stream)
{
    return IAasFilePtr();
}

}
