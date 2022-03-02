#pragma once

#include <functional>
#include <iosfwd>
#include "ParticleDef.h"
#include "ThreadedDefLoader.h"
#include "parser/DefTokeniser.h"

namespace particles
{

class ParticleLoader :
    public util::ThreadedDefLoader<void>
{
private:
    std::function<ParticleDefPtr(const std::string&)> _findOrInsert;
    std::function<void()> _onFinished;

public:
    ParticleLoader(const std::function<ParticleDefPtr(const std::string&)>& findOrInsert,
        const std::function<void()>& onFinished) :
        ThreadedDefLoader(PARTICLES_DIR, PARTICLES_EXT, 1, std::bind(&ParticleLoader::load, this), onFinished),
        _findOrInsert(findOrInsert)
    {}

private:
    void load();

    /**
    * Accept a stream containing particle definitions to parse and add to the list.
    */
    void parseStream(std::istream& contents, const std::string& filename);

    void parseParticleDef(parser::DefTokeniser& tok, const std::string& filename);
};

}
