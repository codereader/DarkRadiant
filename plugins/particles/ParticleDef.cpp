#include "ParticleDef.h"

#include <boost/make_shared.hpp>

using namespace boost;

namespace particles
{

std::size_t ParticleDef::addParticleStage()
{
    // Create a new stage and relay its changed signal
    ParticleStagePtr stage = make_shared<ParticleStage>();
    stage->signal_changed().connect(_changedSignal.make_slot());
    _stages.push_back(stage);

    _changedSignal.emit();

    return _stages.size() - 1;
}

void ParticleDef::removeParticleStage(std::size_t index)
{
    if (index < _stages.size())
    {
        _stages.erase(_stages.begin() + index);
    }

    _changedSignal.emit();
}

void ParticleDef::swapParticleStages(std::size_t index, std::size_t index2)
{
    if (index >= _stages.size() || index2 >= _stages.size() || index == index2)
    {
        return;
    }

    std::swap(_stages[index], _stages[index2]);
    _changedSignal.emit();
}

void ParticleDef::appendStage(const ParticleStagePtr& stage)
{
    // Relay the incoming stage's changed signal then add to list
    stage->signal_changed().connect(_changedSignal.make_slot());
    _stages.push_back(stage);
    _changedSignal.emit();
}

void ParticleDef::copyFrom(const IParticleDef& other)
{
    setDepthHack(other.getDepthHack());

    _stages.clear();

    // Copy each stage
    for (std::size_t i = 0; i < other.getNumStages(); ++i)
    {
        ParticleStagePtr stage = make_shared<ParticleStage>();
        stage->copyFrom(other.getParticleStage(i));
        stage->signal_changed().connect(_changedSignal.make_slot());
        _stages.push_back(stage);
    }
}

void ParticleDef::parseFromTokens(parser::DefTokeniser& tok)
{
    // Clear out the particle def (except the name) before parsing
    clear();

    // Any global keywords will come first, after which we get a series of
    // brace-delimited stages.
    std::string token = tok.nextToken();

    while (token != "}")
    {
        if (token == "depthHack")
        {
            setDepthHack(strToFloat(tok.nextToken()));
        }
        else if (token == "{")
        {
            // Construct/Parse the stage from the tokens
            ParticleStagePtr stage = make_shared<ParticleStage>(ref(tok));

            // Append to the ParticleDef
            appendStage(stage);
        }

        // Get next token
        token = tok.nextToken();
    }

    _changedSignal.emit();
}

}
