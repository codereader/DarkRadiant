#include "RotationKey.h"

#include "gamelib.h"
#include "math/Matrix4.h"

namespace entity
{

namespace
{
    const char* GKEY_ROTATIONKEY = "/mapFormat/entityRotationKey";

    const char* ANGLES_KEY = "angles";
    const char* ROTATION_KEY = "rotation";
}

void RotationKey::write(Entity* entity, bool isModel) const
{
    Vector3 euler = m_rotation.getMatrix4().getEulerAnglesXYZDegrees();

    // greebo: Prevent the "angle" key from being used for models, they should
    // always have a rotation matrix written to their spawnargs. This should
    // fix the models hopping around after transforms
    if (!isModel)
    {
        if (euler[0] == 0 && euler[1] == 0)
        {
            entity->setKeyValue(ROTATION_KEY, "");
            AngleKey::writeToEntity(euler[2], entity);
            return;
        }

        // Non-z-rotations will fall through here
    }

    // The below call to reset the "angle" keyvalue will 
    // trigger callbacks and reset this matrix.
    std::string keyVal = m_rotation.getRotationKeyValue();

    entity->setKeyValue("angle", "");

    // Set an angles key if the game calls for it
    if (game::current::getValue<std::string>(GKEY_ROTATIONKEY) == ANGLES_KEY)
    {
        std::string anglesVal = std::to_string(euler[1]) + ' '
                              + std::to_string(euler[2]) + ' '
                              + std::to_string(euler[0]);

        entity->setKeyValue(ANGLES_KEY, anglesVal);
    }

    // This call will usually update the m_rotation member again
    entity->setKeyValue(ROTATION_KEY, keyVal);
}

} // namespace
