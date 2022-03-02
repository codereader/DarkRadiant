#pragma once

#include <functional>
#include <iosfwd>
#include "ParticleDef.h"
#include "parser/ThreadedDeclParser.h"
#include "parser/DefTokeniser.h"

namespace particles
{

using ParticleDefMap = std::map<std::string, ParticleDefPtr>;

class ParticleLoader :
    public parser::ThreadedDeclParser<void>
{
private:
    ParticleDefMap& _particles;

    // A unique parse pass identifier, used to check when existing
    // definitions have been parsed
    std::size_t _curParseStamp;

public:
    ParticleLoader(ParticleDefMap& particles) :
        parser::ThreadedDeclParser<void>(decl::Type::Particle, PARTICLES_DIR, PARTICLES_EXT, 1),
        _particles(particles),
        _curParseStamp(0)
    {}

protected:
    void onBeginParsing() override;
    void parse(std::istream& stream, const vfs::FileInfo& fileInfo, const std::string& modDir) override;
    void onFinishParsing() override;

private:
    void parseParticleDef(parser::DefTokeniser& tok, const std::string& filename);
};

}
