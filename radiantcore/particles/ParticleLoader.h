#pragma once

#include <functional>
#include <iosfwd>
#include "ParticleDef.h"
#include "parser/ThreadedDeclParser.h"
#include "parser/DefTokeniser.h"

namespace particles
{

class ParticleLoader :
    public parser::ThreadedDeclParser<void>
{
private:
    std::function<ParticleDefPtr(const std::string&)> _findOrInsert;

public:
    ParticleLoader(const std::function<ParticleDefPtr(const std::string&)>& findOrInsert) :
        parser::ThreadedDeclParser<void>(decl::Type::Particle, PARTICLES_DIR, PARTICLES_EXT, 1),
        _findOrInsert(findOrInsert)
    {}

protected:
    void parse(std::istream& stream, const vfs::FileInfo& fileInfo, const std::string& modDir) override;

private:
    // Accept a stream containing particle definitions to parse and add to the list.
    void parseStream(std::istream& contents, const std::string& filename);
    void parseParticleDef(parser::DefTokeniser& tok, const std::string& filename);
};

}
