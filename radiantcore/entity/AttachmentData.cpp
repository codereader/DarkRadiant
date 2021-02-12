#include "AttachmentData.h"

#include <string/predicate.h>
#include <string/convert.h>

namespace entity
{

namespace
{

// Constants
const std::string DEF_ATTACH = "def_attach";
const std::string NAME_ATTACH = "name_attach";
const std::string POS_ATTACH = "pos_attach";

const std::string ATTACH_POS_NAME = "attach_pos_name";
const std::string ATTACH_POS_ORIGIN = "attach_pos_origin";
const std::string ATTACH_POS_JOINT = "attach_pos_joint";
const std::string ATTACH_POS_ANGLES = "attach_pos_angles";

// Extract and return the string suffix for a key (which might be the empty
// string if there is no suffix). Returns false if the key did not match
// the prefix.
bool tryGetSuffixedKey(const std::string& key, const std::string& prefix,
                       std::string& suffixedOutput)
{
    if (string::istarts_with(key, prefix))
    {
		suffixedOutput = key.substr(prefix.length());
		return true;
    }

	suffixedOutput.clear();
	return false;
}

} // namespace

void AttachmentData::parseDefAttachKeys(const std::string& key,
                                        const std::string& value)
{
	std::string suffix;

	if (tryGetSuffixedKey(key, DEF_ATTACH, suffix))
	{
		_objects[suffix].className = value;
	}
	else if (tryGetSuffixedKey(key, NAME_ATTACH, suffix))
	{
		_objects[suffix].name = value;
	}
	else if (tryGetSuffixedKey(key, POS_ATTACH, suffix))
	{
		_objects[suffix].posName = value;
	}
	else if (tryGetSuffixedKey(key, ATTACH_POS_NAME, suffix))
	{
		_positions[suffix].name = value;
	}
	else if (tryGetSuffixedKey(key, ATTACH_POS_ORIGIN, suffix))
	{
		_positions[suffix].origin = string::convert<Vector3>(value);
	}
	else if (tryGetSuffixedKey(key, ATTACH_POS_ANGLES, suffix))
	{
		_positions[suffix].angles = string::convert<Vector3>(value);
	}
	else if (tryGetSuffixedKey(key, ATTACH_POS_JOINT, suffix))
	{
		_positions[suffix].joint = value;
	}
}

void AttachmentData::validateAttachments()
{
    // During parsing we indexed spawnargs by string suffix so that matching
    // keys could be found. From now on we are no longer interested in the
    // suffixes so we will re-build the maps indexed by name instead.
    reindexMapByName(_objects);
    reindexMapByName(_positions);

    // Drop any attached objects that specify a non-existent position (I
    // assume new positions cannot be dynamically created in game).
    for (AttachedObjects::iterator i = _objects.begin();
            i != _objects.end();
            /* in-loop increment */)
    {
        if (_positions.find(i->second.posName) == _positions.end())
        {
            rWarning() << "[AttachmentData] Entity '" << _entityName
                       << "' tries to attach '" << i->first
                       << "' at non-existent position '" << i->second.posName
                       << "'\n";

            _objects.erase(i++);
        }
        else
        {
            ++i;
        }
    }
}

}