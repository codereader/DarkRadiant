#include "ParticlesManager.h"

#include "ParticleDef.h"
#include "ParticleNode.h"
#include "RenderableParticle.h"

#include "icommandsystem.h"
#include "itextstream.h"
#include "ifiletypes.h"
#include "ideclmanager.h"
#include "i18n.h"

#include <functional>

#include "decl/DeclarationCreator.h"
#include "string/predicate.h"
#include "module/StaticModule.h"

namespace particles
{

/* CONSTANTS */
constexpr const char* const PARTICLES_DIR = "particles/";
constexpr const char* const PARTICLES_EXT = ".prt";

sigc::signal<void>& ParticlesManager::signal_particlesReloaded()
{
    return _particlesReloadedSignal;
}

// Visit all of the particle defs
void ParticlesManager::forEachParticleDef(const ParticleDefVisitor& visitor)
{
    GlobalDeclarationManager().foreachDeclaration(decl::Type::Particle, [&](const decl::IDeclaration::Ptr& decl)
    {
        visitor(*std::static_pointer_cast<IParticleDef>(decl));
    });
}

IParticleDef::Ptr ParticlesManager::getDefByName(const std::string& name)
{
    return std::static_pointer_cast<IParticleDef>(
        GlobalDeclarationManager().findDeclaration(decl::Type::Particle, name)
    );
}

IParticleNodePtr ParticlesManager::createParticleNode(const std::string& name)
{
	std::string nameCleaned = name;

	// Cut off the ".prt" from the end of the particle name
	if (string::ends_with(nameCleaned, ".prt"))
	{
		nameCleaned = nameCleaned.substr(0, nameCleaned.length() - 4);
	}

    auto def = getDefByName(nameCleaned);

    if (!def)
    {
        return IParticleNodePtr();
    }

	return std::make_shared<ParticleNode>(std::make_shared<RenderableParticle>(def));
}

IRenderableParticlePtr ParticlesManager::getRenderableParticle(const std::string& name)
{
    auto def = getDefByName(name);

    return def ? std::make_shared<RenderableParticle>(def) : IRenderableParticlePtr();
}

IParticleDef::Ptr ParticlesManager::findOrInsertParticleDef(const std::string& name)
{
    return findOrInsertParticleDefInternal(name);
}

ParticleDefPtr ParticlesManager::findOrInsertParticleDefInternal(const std::string& name)
{
    return std::static_pointer_cast<ParticleDef>(
        GlobalDeclarationManager().findOrCreateDeclaration(decl::Type::Particle, name)
    );
}

void ParticlesManager::removeParticleDef(const std::string& name)
{
    GlobalDeclarationManager().removeDeclaration(decl::Type::Particle, name);
}

const std::string& ParticlesManager::getName() const
{
	static std::string _name(MODULE_PARTICLESMANAGER);
	return _name;
}

const StringSet& ParticlesManager::getDependencies() const
{
    static StringSet _dependencies
    {
        MODULE_DECLMANAGER,
        MODULE_COMMANDSYSTEM,
        MODULE_FILETYPES,
    };

	return _dependencies;
}

void ParticlesManager::initialiseModule(const IApplicationContext& ctx)
{
    GlobalDeclarationManager().registerDeclType("particle", std::make_shared<decl::DeclarationCreator<ParticleDef>>(decl::Type::Particle));
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Particle, PARTICLES_DIR, PARTICLES_EXT);

	// Register the particle file extension
	GlobalFiletypes().registerPattern("particle", FileTypePattern(_("Particle File"), "prt", "*.prt"));

    _defsReloadedConn = GlobalDeclarationManager().signal_DeclsReloaded(decl::Type::Particle).connect(
        [this]() { _particlesReloadedSignal.emit(); }
    );
}

void ParticlesManager::shutdownModule()
{
    _defsReloadedConn.disconnect();
}

void ParticlesManager::saveParticleDef(const std::string& particleName)
{
    auto decl = getDefByName(particleName);

    if (!decl)
    {
        throw std::runtime_error(_("Cannot save particle, it has not been registered yet."));
    }

    GlobalDeclarationManager().saveDeclaration(decl);
}

module::StaticModuleRegistration<ParticlesManager> particlesManagerModule;

} // namespace particles
