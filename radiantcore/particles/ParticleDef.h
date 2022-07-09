#pragma once

#include <vector>

#include "iparticles.h"

#include "StageDef.h"
#include "string/string.h"
#include "parser/DefTokeniser.h"
#include "DeclarationBase.h"

namespace particles
{

/**
 * Representation of a single particle definition. Each definition is comprised
 * of a number of "stages", which must all be rendered in turn.
 */
class ParticleDef :
    public decl::DeclarationBase<IParticleDef>
{
	// The filename this particle has been defined in
	std::string _filename;

	// Depth hack
	float _depthHack;

	// Vector of stages
    std::vector<IStageDef::Ptr> _stages;

    // Changed signal
    sigc::signal<void> _changedSignal;

public:

	/**
	 * Construct a named ParticleDef.
	 */
	ParticleDef(const std::string& name) :
        DeclarationBase<particles::IParticleDef>(decl::Type::Particle, name),
        _depthHack(0)
	{}

	/**
	 * Return the ParticleDef name.
	 */
	const std::string& getName() const override
	{
		return getDeclName();
	}

	const std::string& getFilename() const override
	{
		return _filename;
	}

	void setFilename(const std::string& filename) override
	{
		_filename = filename;
	}

	// Clears stage and depth hack information
	// Name and observers are NOT cleared
	void clear()
	{
		_depthHack = false;
		_stages.clear();
	}

    // IParticleDef implementation
    sigc::signal<void>& signal_changed() override 
	{ 
		return _changedSignal;
	}

	float getDepthHack() override
	{
        ensureParsed();
		return _depthHack;
	}

	void setDepthHack(float value) override
	{
        ensureParsed();
		_depthHack = value;
	}

	std::size_t getNumStages() override
	{
        ensureParsed();
		return _stages.size();
	}

    const std::shared_ptr<IStageDef>& getStage(std::size_t stageNum) override
	{
        ensureParsed();
		return _stages[stageNum];
	}

	std::size_t addParticleStage() override;

	void removeParticleStage(std::size_t index) override;

	void swapParticleStages(std::size_t index, std::size_t index2) override;

	void appendStage(const StageDef::Ptr& stage);

	bool isEqualTo(Ptr& other) override
	{
		// Compare depth hack flag
		if (getDepthHack() != other->getDepthHack()) return false;

		// Compare number of stages
		if (getNumStages() != other->getNumStages()) return false;

		// Compare each stage
		for (std::size_t i = 0; i < getNumStages(); ++i)
		{
			if (getStage(i) != other->getStage(i)) return false;
		}

		// All checks passed => equal
		return true;
	}

	void copyFrom(const Ptr& other) override;

	// Stream insertion operator, writing the entire particle def to the given stream
	friend std::ostream& operator<< (std::ostream& stream, const ParticleDef& def);

protected:
    void onBeginParsing() override;
	void parseFromTokens(parser::DefTokeniser& tok) override;
    void onParsingFinished() override;
};
typedef std::shared_ptr<ParticleDef> ParticleDefPtr;

// This will write the entire particle decl to the given stream, including the leading "particle" keyword
inline std::ostream& operator<<(std::ostream& stream, const ParticleDef& def)
{
	// Don't use scientific notation when exporting floats
	stream << std::fixed;

	// Decl keyword, name and opening brace
	stream << "particle " << def.getName() << " { " << std::endl;

	// Write stages, one by one
	for (const auto& stage : def._stages)
	{
		stream << *std::static_pointer_cast<StageDef>(stage);
	}

	// Closing brace
	stream << "}";

	return stream;
}

}
