#include "Doom3AasFile.h"

#include "itextstream.h"

namespace map
{

void Doom3AasFile::parseFromTokens(parser::DefTokeniser& tok)
{
    while (tok.hasMoreTokens())
    {
        std::string token = tok.nextToken();

        if (token == "settings")
        {
            _settings.parseFromTokens(tok);
        }
        // TODO: Planes, etc.
        else
        {
            throw parser::ParseException("Unknown token: " + token);
        }
    }
}

}
