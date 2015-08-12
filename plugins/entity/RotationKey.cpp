#include "RotationKey.h"

void RotationKey::write(Entity* entity, bool isModel) const
{
    // greebo: Prevent the "angle" key from being used for models, they should always
    // have a rotation matrix written to their spawnargs. This should fix
    // the models hopping around after transforms
    if (!isModel)
    {
        Vector3 euler = m_rotation.getMatrix4().getEulerAnglesXYZDegrees();

        if (euler[0] == 0 && euler[1] == 0)
        {
            entity->setKeyValue("rotation", "");
            write_angle(euler[2], entity);
            return;
        }

        // Non-z-rotations will fall through here
    }

    // The below call to reset the "angle" keyvalue will 
    // trigger callbacks and reset this matrix.
    std::string keyVal = m_rotation.getRotationKeyValue();

    entity->setKeyValue("angle", "");

    // This call will usually update the m_rotation member again
    entity->setKeyValue("rotation", keyVal);
}

