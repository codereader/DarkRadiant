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

    // Find any existing particle def
    auto existing = _particles.try_emplace(name, std::make_shared<ParticleDef>(name));

    if (!existing.second && existing.first->second->getParseStamp() == _curParseStamp)
    {
        rWarning() << "Particle " << name << " already defined in " <<
            existing.first->second->getFilename() <<
            ", ignoring definition in " << filename << std::endl;

        // Use a dummy particle def to consume the rest of this block
        ParticleDef("").parseFromTokens(tok);
        return;
    }

    existing.first->second->setParseStamp(_curParseStamp);
    existing.first->second->setFilename(filename);

    // Let the particle construct itself from the token stream
    existing.first->second->parseFromTokens(tok);
}

void ParticleLoader::onBeginParsing()
{
    ++_curParseStamp;
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

void ParticleLoader::onFinishParsing()
{
    rMessage() << "Found " << _particles.size() << " particle definitions." << std::endl;
}

}
