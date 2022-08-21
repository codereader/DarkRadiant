#pragma once

#include <map>
#include <string>
#include "math/Vector3.h"

#include <ientity.h>

namespace entity
{

/// Representation of parsed `def_attach` and related keys
class AttachmentData
{
    // Name of the entity class being parsed (for debug/error purposes)
    std::string _entityName;

    // Any def_attached entities. Each attachment has an entity class, a
    // position and optionally a name.
    struct Attachment
    {
        // Class of entity that is attached
        std::string className;

        // Name of the entity that is attached
        std::string name;

        // Name of the position (AttachPos) at which the entity should be
        // attached
        std::string posName;
    };

    // Attached object map initially indexed by key suffix (e.g. "1" for
    // "name_attach1"), then by name.
    typedef std::map<std::string, Attachment> AttachedObjects;
    AttachedObjects _objects;

    // Positions at which def_attached entities can be attached.
    struct AttachPos
    {
        // Name of this attachment position (referred to in the
        // Attachment::posName variable)
        std::string name;

        // 3D offset position from our origin or the model joint, if a joint is
        // specified
        Vector3 origin;

        // Rotation of the attached entity
        Vector3 angles;

        // Optional model joint relative to which the origin should be
        // calculated
        std::string joint;
    };

    // Attach position map initially indexed by key suffix (e.g. "_zhandr" for
    // "attach_pos_name_zhandr"), then by name. It appears that only attachpos
    // keys are using arbitrary strings instead of numeric suffixes, but we
    // might as well treat everything the same way.
    typedef std::map<std::string, AttachPos> AttachPositions;
    AttachPositions _positions;

private:

    template<typename Map> void reindexMapByName(Map& inputMap)
    {
        Map copy(inputMap);
        inputMap.clear();

        // Take each item from the copied map, and insert it into the original
        // map using the name as the key.
        for (typename Map::value_type pair : copy)
        {
            if (!pair.second.name.empty()) // ignore empty names
            {
                inputMap.insert(
                    typename Map::value_type(pair.second.name, pair.second)
                );
            }
        }
    }

public:

    /// Initialise and set classname
    AttachmentData(const std::string& name)
    : _entityName(name)
    { }

    /// Clear all data
    void clear()
    {
        _objects.clear();
        _positions.clear();
    }

    /// Attempt to extract attachment data from the given key/value pair
    void parseDefAttachKeys(const std::string& key, const std::string& value);

    /// Sanitise and resolve attachments and their named positions
    void validateAttachments();

    /// Invoke a functor for each attachment
    template<typename Functor>
    void forEachAttachment(Functor func) const
    {
        for (auto i = _objects.begin(); i != _objects.end(); ++i)
        {
            // Locate attachment position
            const AttachPos& pos = _positions.at(i->second.posName);

            // Construct the functor argument
            Entity::Attachment a;
            a.eclass = i->second.className;
            a.name = i->second.name;
            a.offset = pos.origin;
            a.joint = pos.joint;

            // Invoke the functor
            func(a);
        }
    }
};

}