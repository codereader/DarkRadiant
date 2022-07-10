#include "ParticleDef.h"
#include "string/convert.h"
#include "util/ScopedBoolLock.h"

namespace particles
{

sigc::signal<void>& ParticleDef::signal_changed()
{ 
	return _changedSignal;
}

float ParticleDef::getDepthHack()
{
    ensureParsed();
	return _depthHack;
}

void ParticleDef::setDepthHack(float value)
{
    ensureParsed();
    _depthHack = value;
    onParsedContentsChanged();
}

std::size_t ParticleDef::getNumStages()
{
    ensureParsed();
	return _stages.size();
}

const std::shared_ptr<IStageDef>& ParticleDef::getStage(std::size_t stageNum)
{
    ensureParsed();
	return _stages[stageNum];
}

std::size_t ParticleDef::addParticleStage()
{
    // Create a new stage and relay its changed signal
    auto stage = std::make_shared<StageDef>();
    stage->signal_changed().connect(_changedSignal.make_slot());
    _stages.push_back(stage);

    onParticleChanged();

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

void ParticleDef::appendStage(const StageDef::Ptr& stage)
{
    // Relay the incoming stage's changed signal then add to list
    stage->signal_changed().connect(_changedSignal.make_slot());
    _stages.push_back(stage);
    _changedSignal.emit();
}

void ParticleDef::copyFrom(const Ptr& other)
{
    ensureParsed();

    {
        // Suppress signals until we're done copying
        util::ScopedBoolLock changeLock(_blockChangedSignal);

        setDepthHack(other->getDepthHack());

        _stages.clear();

        // Copy each stage
        for (std::size_t i = 0; i < other->getNumStages(); ++i)
        {
            auto stage = std::make_shared<StageDef>();
            stage->copyFrom(other->getStage(i));
            stage->signal_changed().connect(_changedSignal.make_slot());
            _stages.push_back(stage);
        }
    }

	// We've changed all the stages, so emit the changed signal now (#4411)
	onParticleChanged();
}

bool ParticleDef::isEqualTo(const Ptr& other)
{
    // Compare depth hack flag
    if (getDepthHack() != other->getDepthHack()) return false;

    // Compare number of stages
    if (getNumStages() != other->getNumStages()) return false;

    // Compare each stage
    for (std::size_t i = 0; i < getNumStages(); ++i)
    {
        if (!getStage(i)->isEqualTo(other->getStage(i))) return false;
    }

    // All checks passed => equal
    return true;
}

void ParticleDef::onBeginParsing()
{
    // Clear out the particle def (except the name) before parsing
    clear();
}

void ParticleDef::parseFromTokens(parser::DefTokeniser& tokeniser)
{
    // Any global keywords will come first, after which we get a series of
    // brace-delimited stages.

    while (tokeniser.hasMoreTokens())
    {
        auto token = tokeniser.nextToken();

        if (token == "depthHack")
        {
            setDepthHack(string::convert<float>(tokeniser.nextToken()));
        }
        else if (token == "{")
        {
            // Construct/Parse the stage from the tokens
            // Append to the ParticleDef
            appendStage(StageDef::Parse(tokeniser));
        }
    }
}

void ParticleDef::onParsingFinished()
{
    _changedSignal.emit();
}

std::string ParticleDef::generateSyntax()
{
    std::stringstream stream;

    // Don't use scientific notation when exporting floats
    stream << std::fixed;
    stream.precision(3); // Three post-comma digits precision

    stream << "\n"; // initial line break after the opening brace

    if (_depthHack > 0)
    {
        stream << "\tdepthHack\t" << _depthHack << std::endl;
    }

    // Write stages, one by one
    for (const auto& stage : _stages)
    {
        stream << *std::static_pointer_cast<StageDef>(stage);
    }

    stream << "\n"; // final line break before the closing brace

    return stream.str();
}

void ParticleDef::onParticleChanged()
{
    if (_blockChangedSignal) return;

    onParsedContentsChanged();
    _changedSignal.emit();
}

}
