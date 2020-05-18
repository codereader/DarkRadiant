#include "RotationMatrix.h"

#include "ientity.h"
#include "math/Matrix4.h"

void RotationMatrix::setIdentity()
{
    rotation[0] = 1;
    rotation[1] = 0;
    rotation[2] = 0;
    rotation[3] = 0;
    rotation[4] = 1;
    rotation[5] = 0;
    rotation[6] = 0;
    rotation[7] = 0;
    rotation[8] = 1;
}

void RotationMatrix::readFromString(const std::string& value)
{
    std::stringstream strm(value);
    strm << std::skipws;

    for (int i = 0; i < 9; ++i)
    {
        strm >> rotation[i];
    }

    if (!strm)
    {
        // Parsing failed, fall back to the identity matrix
        setIdentity();
    }
}

void RotationMatrix::writeToEntity(Entity* entity, const std::string& key) const
{
    if (rotation[0] == 1 &&
        rotation[1] == 0 &&
        rotation[2] == 0 &&
        rotation[3] == 0 &&
        rotation[4] == 1 &&
        rotation[5] == 0 &&
        rotation[6] == 0 &&
        rotation[7] == 0 &&
        rotation[8] == 1)
    {
        entity->setKeyValue(key, "");
    }
    else
    {
        entity->setKeyValue(key, getRotationKeyValue());
    }
}

std::string RotationMatrix::getRotationKeyValue() const
{
    std::ostringstream value;

    value << rotation[0] << ' '
        << rotation[1] << ' '
        << rotation[2] << ' '
        << rotation[3] << ' '
        << rotation[4] << ' '
        << rotation[5] << ' '
        << rotation[6] << ' '
        << rotation[7] << ' '
        << rotation[8];

    return value.str();
}

Matrix4 RotationMatrix::getMatrix4() const
{
    return Matrix4::byColumns(
        rotation[0],
        rotation[1],
        rotation[2],
        0,
        rotation[3],
        rotation[4],
        rotation[5],
        0,
        rotation[6],
        rotation[7],
        rotation[8],
        0,
        0,
        0,
        0,
        1
    );
}

void RotationMatrix::setFromMatrix4(const Matrix4& matrix)
{
    rotation[0] = static_cast<float>(matrix.xx());
    rotation[1] = static_cast<float>(matrix.xy());
    rotation[2] = static_cast<float>(matrix.xz());
    rotation[3] = static_cast<float>(matrix.yx());
    rotation[4] = static_cast<float>(matrix.yy());
    rotation[5] = static_cast<float>(matrix.yz());
    rotation[6] = static_cast<float>(matrix.zx());
    rotation[7] = static_cast<float>(matrix.zy());
    rotation[8] = static_cast<float>(matrix.zz());
}

// Assignment operator
const RotationMatrix& RotationMatrix::operator=(const RotationMatrix& other)
{
    rotation[0] = other[0];
    rotation[1] = other[1];
    rotation[2] = other[2];
    rotation[3] = other[3];
    rotation[4] = other[4];
    rotation[5] = other[5];
    rotation[6] = other[6];
    rotation[7] = other[7];
    rotation[8] = other[8];

    return *this;
}

void RotationMatrix::rotate(const Quaternion& rotate)
{
    setFromMatrix4(
        getMatrix4().getPremultipliedBy(Matrix4::getRotationQuantised(rotate))
    );
}

void RotationMatrix::setFromAngleString(const std::string& value)
{
    try
    {
        float angle = std::stof(value);

        // Cast succeeded
        setFromMatrix4(Matrix4::getRotationAboutZDegrees(angle));
    }
    catch (std::invalid_argument&)
    {
        // Cast failed
        setIdentity();
    }
}
