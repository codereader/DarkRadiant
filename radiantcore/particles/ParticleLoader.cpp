#include "ParticleLoader.h"

#include "ifilesystem.h"
#include "itextstream.h"
#include "debugging/ScopedDebugTimer.h"

namespace particles
{

void ParticleLoader::parseParticleDef(parser::DefTokeniser& tok, const std::string& filename)
{
    // Standard DEF, starts with "particle <name> {"
    auto declName = tok.nextToken();

    // Check for a valid particle declaration, some .prt files contain materials
    if (declName != "particle")
    {
        // No particle, skip name plus whole block
        tok.skipTokens(1);
        tok.assertNextToken("{");

        for (std::size_t level = 1; level > 0;)
        {
            std::string token = tok.nextToken();

            if (token == "}")
            {
                level--;
            }
            else if (token == "{")
            {
                level++;
            }
        }

        return;
    }

    // Valid particle declaration, go ahead parsing the name
    auto name = tok.nextToken();
    tok.assertNextToken("{");

    // Find the particle def (use the non-blocking, internal lookup)
    auto def = _findOrInsert(name);

    def->setFilename(filename);

    // Let the particle construct itself from the token stream
    def->parseFromTokens(tok);
}

void ParticleLoader::parse(std::istream& stream, const vfs::FileInfo& fileInfo, const std::string& modDir)
{
    // Usual ritual, get a parser::DefTokeniser and start tokenising the DEFs
    parser::BasicDefTokeniser<std::istream> tok(stream);

    while (tok.hasMoreTokens())
    {
        parseParticleDef(tok, fileInfo.name);
    }
}

}
