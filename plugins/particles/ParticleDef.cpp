#include "ParticleDef.h"
#include "string/convert.h"

#include <boost/make_shared.hpp>

using namespace boost;

namespace particles
{

std::size_t ParticleDef::addParticleStage()
{
    // Create a new stage and relay its changed signal
    StageDefPtr stage = make_shared<StageDef>();
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

void ParticleDef::appendStage(const StageDefPtr& stage)
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
        StageDefPtr stage = make_shared<StageDef>();
        stage->copyFrom(other.getStage(i));
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
            setDepthHack(string::convert<float>(tok.nextToken()));
        }
        else if (token == "{")
        {
            // Construct/Parse the stage from the tokens
            StageDefPtr stage = make_shared<StageDef>(ref(tok));

            // Append to the ParticleDef
            appendStage(stage);
        }

        // Get next token
        token = tok.nextToken();
    }

    _changedSignal.emit();
}

}
