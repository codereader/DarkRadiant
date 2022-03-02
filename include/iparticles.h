#pragma once

#include "imodule.h"
#include "irenderable.h"

#include <functional>
#include <sigc++/signal.h>

class RenderSystem;
class Matrix4;
template<typename Element> class BasicVector3;
typedef BasicVector3<double> Vector3;
class AABB;

namespace scene
{
	class INode;
	typedef std::shared_ptr<INode> INodePtr;
}

/// Classes related to storage and rendering of particle systems
namespace particles
{

// see iparticlestage.h for definition
class IStageDef;

// see iparticlenode.h for definition
class IParticleNode;
typedef std::shared_ptr<IParticleNode> IParticleNodePtr;

/**
 * \brief
 * Definition of a particle system.
 *
 * Each particle system is made up of one or more particle stages, information
 * about which is provided via the IStageDef interface.
 */
class IParticleDef
{
public:

    /**
	 * Destructor
	 */
	virtual ~IParticleDef() {}

	/// Get the name of the particle system.
	virtual const std::string& getName() const = 0;

	/**
	 * Get the name of the .prt file this particle is defined in.
	 * Might return an empty string if this particle def has not been saved yet.
	 */
	virtual const std::string& getFilename() const = 0;

	virtual void setFilename(const std::string& filename) = 0;

	/**
	 * Set/get the depth hack flag
	 */
	virtual float getDepthHack() const = 0;
	virtual void setDepthHack(float value) = 0;

	/// Returns the number of stages for this particle system.
	virtual std::size_t getNumStages() const = 0;

    /// Get a const stage definition from the particle definition
	virtual const IStageDef& getStage(std::size_t stageNum) const = 0;

    /// Get a stage definition from the particle definition
	virtual IStageDef& getStage(std::size_t stageNum) = 0;

	/**
	 * Add a new stage to this particle, returns the index of the new stage.
	 */
	virtual std::size_t addParticleStage() = 0;

	/**
	 * Removes the stage with the given index.
	 */
	virtual void removeParticleStage(std::size_t index) = 0;

	/**
	 * Swaps the location of the two given particle stages. After this step the
	 * particle stage at <index> will be at <index2> and vice versa.
	 * If one of the indices is out of bounds (or both indices are equal) nothing will happen.
	 */
	virtual void swapParticleStages(std::size_t index, std::size_t index2) = 0;

    /// Signal emitted when some aspect of the particle def has changed
    virtual sigc::signal<void>& signal_changed() = 0;

	// Comparison operators - particle defs are considered equal if all properties (except the name!),
	// number of stages and stage contents are the equal
	virtual bool operator==(const IParticleDef& other) const = 0;
	virtual bool operator!=(const IParticleDef& other) const = 0;

	// Copies all properties from the other particle, overwriting this one
	// Note: Name, filename and observers are not copied
	virtual void copyFrom(const IParticleDef& other) = 0;
};
typedef std::shared_ptr<IParticleDef> IParticleDefPtr;

/**
 * A renderable particle, which is capable of compiling the
 * particle system into actual geometry usable for the backend rendersystem.
 *
 * As it derives from Renderable, this object can be added to a RenderableCollector
 * during the front-end render phase.
 */
class IRenderableParticle :
	public Renderable
{
public:
	/**
	 * Update the particle geometry using the given rendersystem.
	 * The rendersystem is needed for acquiring the shaders and
	 * the current render time.
	 *
	 * @viewRotation: the matrix to orient themselves to the viewer.
     * @localToWorld: the combined transform of local to world coordinates
     * @entity: the parent entity
	 */
	virtual void update(const Matrix4& viewRotation, const Matrix4& localToWorld, IRenderEntity* entity) = 0;
		
	/**
	 * Get the particle definition used by this renderable.
	 */
	virtual const IParticleDefPtr& getParticleDef() const = 0;

	/**
	 * Set the particle definition. You'll need to call update() after
	 * setting a new particle def.
	 */
	virtual void setParticleDef(const IParticleDefPtr& def) = 0;

	/**
	 * greebo: Particles have a main direction, usually defined by the
	 * emitter's rotation. For a stand-alone particle (without emitter)
	 * this direction defaults to <0,0,1>, but can be overridden here.
	 */
	virtual void setMainDirection(const Vector3& direction) = 0;

	/**
	 * Set the colour needed by the particle system when the setting
	 * "use entity colour" is activated.
	 */
	virtual void setEntityColour(const Vector3& colour) = 0;

	/**
	 * Returns the bounding box taken by the entirety of quads in this particle.
	 * Make sure to call this after the update() method, as getAABB() will
	 * calculate and return the bounds at the time passed to update().
	 */
	virtual const AABB& getBounds() = 0;
};
typedef std::shared_ptr<IRenderableParticle> IRenderableParticlePtr;

/**
 * Callback for evaluation particle defs.
 */
typedef std::function< void (const IParticleDef&) > ParticleDefVisitor;

/* CONSTANTS */
namespace
{
	const char* PARTICLES_DIR = "particles/";
	const char* PARTICLES_EXT = "prt";
}

/// Inteface for the particles manager
class IParticlesManager :
	public RegisterableModule
{
public:

    /// Signal emitted when particle definitions are reloaded
    virtual sigc::signal<void>& signal_particlesReloaded() = 0;

	/// Enumerate each particle def.
	virtual void forEachParticleDef(const ParticleDefVisitor&) = 0;

    /// Return the definition object for the given named particle system
	virtual IParticleDefPtr getDefByName(const std::string& name) = 0;

	// Finds or creates the particle def with the given name, always returns non-NULL
	virtual IParticleDefPtr findOrInsertParticleDef(const std::string& name) = 0;

	// Removes the named particle definition from the storage
	virtual void removeParticleDef(const std::string& name) = 0;

	/**
	 * Create a renderable particle, which is capable of compiling the
	 * particle system into actual geometry usable for the backend rendersystem.
	 *
	 * @returns: the renderable particle instance or NULL if the named particle was not found.
	 */
	virtual IRenderableParticlePtr getRenderableParticle(const std::string& name) = 0;

    /// Create and return a particle node for the named particle system
	virtual IParticleNodePtr createParticleNode(const std::string& name) = 0;

	/**
	 * Writes the named particle declaration to the file it is associated with,
	 * replacing any existing declaration with the same name in that file.
	 * Any other particle declarations that happen to be declared in the same file
	 * will be preserved.
	 *
	 * If the associated file is stored in a PK4, a copy is written to the current
	 * fs_game ("mod") folder and that file is used for the save operation.
	 * No other particle declaration will be removed from the copied file.
	 *
	 * Note: this method does not check if the named particle system is already
	 * defined in a different file.
	 *
	 * throws a std::runtime_error on any failure.
	 */
	virtual void saveParticleDef(const std::string& particle) = 0;

	/**
     * \brief
     * Force the particles manager to reload all particle definitions from the
     * .prt files.
     *
     * Any existing references to IParticleDefs will remain valid, but their
     * contents might change.  Anything sensitive to these changes (like the
     * renderable particles) should connect to the particles reloaded signal.
	 *
     * If particle defs are removed from the .prt files, the corresponding
     * IParticleDef instance will remain in memory, but will be empty after
     * reload.
	 */
	virtual void reloadParticleDefs() = 0;
};

} // namespace

const char* const MODULE_PARTICLESMANAGER = "ParticlesManager";

// Accessor
inline particles::IParticlesManager& GlobalParticlesManager()
{
	static module::InstanceReference<particles::IParticlesManager> _reference(MODULE_PARTICLESMANAGER);
	return _reference;
}
