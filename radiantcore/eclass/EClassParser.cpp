#include "EClassParser.h"

#include "ieclasscolours.h"
#include "itextstream.h"

#include "string/case_conv.h"
#include "parser/DefTokeniser.h"

namespace eclass
{

void EClassParser::onBeginParsing()
{
    // Increase the parse stamp for this run
    _curParseStamp++;

    // Block load signals until inheritance of all classes has been completed
    // we can't have eclass changed signals emitted before we have that sorted out
    for (const auto& eclass : _entityClasses)
    {
        eclass.second->blockChangedSignal(true);
    }
    
    _owner.defsLoadingSignal().emit();
}

void EClassParser::parse(std::istream& stream, const vfs::FileInfo& fileInfo, const std::string& modDir)
{
    // Construct a tokeniser for the stream
    parser::BasicDefTokeniser<std::istream> tokeniser(stream);

    while (tokeniser.hasMoreTokens())
    {
        std::string blockType = tokeniser.nextToken();
        string::to_lower(blockType);

        if (blockType == "entitydef")
        {
            // Get the (lowercase) entity name
            const std::string sName =
                string::to_lower_copy(tokeniser.nextToken());

            // Ensure that an Entity class with this name already exists
            // When reloading entityDef declarations, most names will already be registered
            auto i = _entityClasses.find(sName);

            if (i == _entityClasses.end())
            {
                // Not existing yet, allocate a new class
                auto result = _entityClasses.emplace(sName, std::make_shared<EntityClass>(sName, fileInfo, false));

                i = result.first;
            }
            else
            {
                // EntityDef already exists, compare the parse stamp
                if (i->second->getParseStamp() == _curParseStamp)
                {
                    rWarning() << "[eclassmgr]: EntityDef "
                        << sName << " redefined" << std::endl;
                }
            }

            // At this point, i is pointing to a valid entityclass
            i->second->setParseStamp(_curParseStamp);

            // Parse the contents of the eclass (excluding name)
            i->second->parseFromTokens(tokeniser);

            // Set the mod directory
            i->second->setModName(modDir);
        }
    }
}

void EClassParser::onFinishParsing()
{
    resolveInheritance();
    applyColours();

    for (const auto& eclass : _entityClasses)
    {
        eclass.second->blockChangedSignal(false);
        eclass.second->emitChangedSignal();
    }

    _owner.defsLoadedSignal().emit();
}

void EClassParser::resolveInheritance()
{
    // Resolve inheritance for the entities. At this stage the classes
    // will have the name of their parent, but not an actual pointer to it
    for (auto& pair : _entityClasses)
    {
        // Tell the class to resolve its own inheritance using the given
        // map as a source for parent lookup
        pair.second->resolveInheritance(_entityClasses);

#if 0
        // If the entity has a model path ("model" key), lookup the actual
        // model and apply its mesh and skin to this entity.
        if (!pair.second->getModelPath().empty())
        {
            auto j = _modelDefs.find(pair.second->getModelPath());

            if (j != _modelDefs.end())
            {
                pair.second->setModelPath(j->second->getMesh());
                pair.second->setSkin(j->second->getSkin());
            }
        }
#endif
    }
}

void EClassParser::applyColours()
{
    GlobalEclassColourManager().foreachOverrideColour([&](const std::string& eclass, const Vector4& colour)
    {
        auto foundEclass = _entityClasses.find(string::to_lower_copy(eclass));

        if (foundEclass != _entityClasses.end())
        {
            foundEclass->second->setColour(colour);
        }
    });
}

}
