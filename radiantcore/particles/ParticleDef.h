#pragma once

#include "StageDef.h"

#include "iparticles.h"

#include "parser/DefTokeniser.h"
#include "string/string.h"
#include <vector>
#include <set>

namespace particles
{

/**
 * Representation of a single particle definition. Each definition is comprised
 * of a number of "stages", which must all be rendered in turn.
 */
class ParticleDef
: public IParticleDef
{
	// Name
	std::string _name;

	// The filename this particle has been defined in
	std::string _filename;

	// Depth hack
	float _depthHack;

	// Vector of stages
	typedef std::vector<StageDefPtr> StageList;
	StageList _stages;

    // Changed signal
    sigc::signal<void> _changedSignal;

    // A unique parse pass identifier
    std::size_t _parseStamp;

public:

	/**
	 * Construct a named ParticleDef.
	 */
	ParticleDef(const std::string& name)
	: _name(name)
	{ }

	/**
	 * Return the ParticleDef name.
	 */
	const std::string& getName() const override
	{
		return _name;
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

	float getDepthHack() const override
	{
		return _depthHack;
	}

	void setDepthHack(float value) override
	{
		_depthHack = value;
	}

	std::size_t getNumStages() const override
	{
		return _stages.size();
	}

	const IStageDef& getStage(std::size_t stageNum) const override
	{
		return *_stages[stageNum];
	}

	IStageDef& getStage(std::size_t stageNum) override
	{
		return *_stages[stageNum];
	}

	std::size_t addParticleStage() override;

	void removeParticleStage(std::size_t index) override;

	void swapParticleStages(std::size_t index, std::size_t index2) override;

	void appendStage(const StageDefPtr& stage);

	bool operator==(const IParticleDef& other) const override
	{
		// Compare depth hack flag
		if (getDepthHack() != other.getDepthHack()) return false;

		// Compare number of stages
		if (getNumStages() != other.getNumStages()) return false;

		// Compare each stage
		for (std::size_t i = 0; i < getNumStages(); ++i)
		{
			if (getStage(i) != other.getStage(i)) return false;
		}

		// All checks passed => equal
		return true;
	}

	bool operator!=(const IParticleDef& other) const override
	{
		return !operator==(other);
	}

	void copyFrom(const IParticleDef& other) override;

	void parseFromTokens(parser::DefTokeniser& tok);

    std::size_t getParseStamp() const
    {
        return _parseStamp;
    }

    void setParseStamp(std::size_t stamp)
    {
        _parseStamp = stamp;
    }

	// Stream insertion operator, writing the entire particle def to the given stream
	friend std::ostream& operator<< (std::ostream& stream, const ParticleDef& def);
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
	for (ParticleDef::StageList::const_iterator i = def._stages.begin(); i != def._stages.end(); ++i)
	{
		const StageDef& stage = **i;
		stream << stage;
	}

	// Closing brace
	stream << "}";

	return stream;
}

}
