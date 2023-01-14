#pragma once

#include <map>
#include <string>
#include <array>
#include "ipatch.h"
#include "ibrush.h"
#include "imodel.h"
#include "iparticlenode.h"
#include "iparticles.h"
#include "iparticlestage.h"
#include "iscenegraph.h"

namespace scene
{

/**
 * greebo: This object traverses the scenegraph on construction
 * counting all occurrences of each shader.
 */
class ShaderBreakdown :
	public scene::NodeVisitor
{
public:
    enum OwnerType
    {
        Face = 0,
        Patch = 1,
        Model = 2,
        Particle = 3
    };

	typedef std::map<std::string, std::array<std::size_t, 4>> Map;

private:
	Map _map;

public:
	ShaderBreakdown()
	{
		_map.clear();
		GlobalSceneGraph().root()->traverseChildren(*this);
	}

	bool pre(const scene::INodePtr& node) override
	{
		// Check if this node is a patch
		if (Node_isPatch(node))
		{
			increaseShaderCount(Node_getIPatch(node)->getShader(), OwnerType::Patch);
			return false;
		}

		if (Node_isBrush(node))
        {
			auto brush = Node_getIBrush(node);
			
			for (std::size_t i = 0; i < brush->getNumFaces(); ++i)
            {
                increaseShaderCount(brush->getFace(i).getShader(), OwnerType::Face);
            }

			return false;
		}

        auto particleNode = std::dynamic_pointer_cast<particles::IParticleNode>(node);

        if (particleNode)
        {
            auto particle = particleNode->getParticle();
            auto particleDef = particle->getParticleDef();

            if (!particleDef) return false;

            std::set<std::string> usedMaterials;

            for (auto i = 0; i < particleDef->getNumStages(); ++i)
            {
                usedMaterials.insert(particleDef->getStage(i)->getMaterialName());
            }
            
            for (const auto& material : usedMaterials)
            {
                increaseShaderCount(material, OwnerType::Particle);
            }

            return false;
        }

        if (Node_isModel(node))
        {
            auto modelNode = Node_getModel(node);

            std::set<std::string> usedMaterials;

            for (const auto& material : modelNode->getIModel().getActiveMaterials())
            {
                usedMaterials.insert(material);
            }

            for (const auto& material : usedMaterials)
            {
                increaseShaderCount(material, OwnerType::Model);
            }

            return false;
        }

		return true;
	}

	// Accessor method to retrieve the shader breakdown map
	const Map& getMap() const
	{
		return _map;
	}

	Map::const_iterator begin() const
	{
		return _map.begin();
	}

	Map::const_iterator end() const
	{
		return _map.end();
	}

private:
	// Local helper to increase the shader occurrence count
	void increaseShaderCount(const std::string& shaderName, OwnerType type)
	{
		// Try to look up the shader in the map
		auto found = _map.find(shaderName);

		if (found == _map.end())
		{
			// Shader not yet registered, create new entry
            auto result = _map.emplace(shaderName, std::array<std::size_t, 4>{ 0, 0, 0, 0 });

			found = result.first;
		}

		// Iterator is valid at this point, increase the counter
		found->second[type]++;
	}

}; // class

} // namespace
