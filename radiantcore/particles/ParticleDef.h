#pragma once

#include <vector>
#include <sigc++/connection.h>

#include "iparticles.h"

#include "StageDef.h"
#include "parser/DefTokeniser.h"
#include "decl/EditableDeclaration.h"

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

	// Vector of stages and the connected change signals
    std::vector<std::pair<IStageDef::Ptr, sigc::connection>> _stages;

    // Changed signal
    sigc::signal<void> _changedSignal;
    bool _blockChangedSignal;

public:
	/**
	 * Construct a named ParticleDef.
	 */
	ParticleDef(const std::string& name) :
        EditableDeclaration<IParticleDef>(decl::Type::Particle, name),
        _depthHack(0),
        _blockChangedSignal(false)
	{}

	void setFilename(const std::string& filename) override
	{
        auto syntax = getBlockSyntax();
        setFileInfo(vfs::FileInfo(syntax.fileInfo.topDir, filename, vfs::Visibility::NORMAL));
	}

	// Clears stage and depth hack information
	// Name and observers are NOT cleared
	void clear()
	{
		_depthHack = false;
		_stages.clear();
	}

    // IParticleDef implementation
    sigc::signal<void>& signal_changed() override;

    float getDepthHack() override;
    void setDepthHack(float value) override;

    std::size_t getNumStages() override;
    const std::shared_ptr<IStageDef>& getStage(std::size_t stageNum) override;
	std::size_t addParticleStage() override;
	void removeParticleStage(std::size_t index) override;
	void swapParticleStages(std::size_t index, std::size_t index2) override;

    bool isEqualTo(const Ptr& other) override;

	void copyFrom(const Ptr& other) override;

	// Stream insertion operator, writing the entire particle def to the given stream
	friend std::ostream& operator<< (std::ostream& stream, ParticleDef& def);

protected:
    void onBeginParsing() override;
	void parseFromTokens(parser::DefTokeniser& tok) override;
    void onSyntaxBlockAssigned(const decl::DeclarationBlockSyntax& block) override;

    std::string generateSyntax() override;

private:
    void onParticleChanged();
    void appendStage(const StageDef::Ptr& stage);
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
