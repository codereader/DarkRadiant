#pragma once

#include <vector>

#include "iparticles.h"

#include "StageDef.h"
#include "parser/DefTokeniser.h"
#include "EditableDeclaration.h"

namespace particles
{

/**
 * Representation of a single particle definition. Each definition is comprised
 * of a number of "stages", which must all be rendered in turn.
 */
class ParticleDef :
    public decl::EditableDeclaration<IParticleDef>
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
        EditableDeclaration<IParticleDef>(decl::Type::Particle, name),
        _depthHack(0)
	{}

	void setFilename(const std::string& filename) override
	{
        auto syntax = getBlockSyntax();
        syntax.fileInfo = vfs::FileInfo(syntax.fileInfo.topDir, filename, vfs::Visibility::NORMAL);
        setBlockSyntax(syntax);
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

    bool isEqualTo(const Ptr& other) override;

	void copyFrom(const Ptr& other) override;

	// Stream insertion operator, writing the entire particle def to the given stream
	friend std::ostream& operator<< (std::ostream& stream, ParticleDef& def);

protected:
    void onBeginParsing() override;
	void parseFromTokens(parser::DefTokeniser& tok) override;
    void onParsingFinished() override;

    std::string generateSyntax() override;
};
typedef std::shared_ptr<ParticleDef> ParticleDefPtr;

// This will write the entire particle decl to the given stream, including the leading "particle" keyword
inline std::ostream& operator<<(std::ostream& stream, ParticleDef& def)
{
	// Decl keyword, name and opening brace
    stream << "particle " << def.getDeclName() << " \n{";

    // The raw contents (contains all needed line breaks)
    stream << def.getBlockSyntax().contents;

	// Closing brace
	stream << "}";

	return stream;
}

}
