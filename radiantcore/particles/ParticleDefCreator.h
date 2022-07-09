#pragma once

#include "ideclmanager.h"
#include "ParticleDef.h"

namespace particles
{

class ParticleDefCreator :
    public decl::IDeclarationCreator
{
public:
    decl::Type getDeclType() const override
    {
        return decl::Type::Particle;
    }

    decl::IDeclaration::Ptr createDeclaration(const std::string& name) override
    {
        return std::make_shared<ParticleDef>(name);
    }
};

}
