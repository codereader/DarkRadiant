/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

namespace math
{
    const double PI = 3.1415926535897932384626433832795;
}

const double c_half_pi = math::PI / 2;
const double c_2pi = 2 * math::PI;

const double c_DEG2RADMULT = math::PI / 180.0;
const double c_RAD2DEGMULT = 180.0 / math::PI;

inline double radians_to_degrees(double radians)
{
  return radians * c_RAD2DEGMULT;
}
inline double degrees_to_radians(double degrees)
{
  return degrees * c_DEG2RADMULT;
}

namespace math
{

/// Class representing an angle in degrees
class Degrees
{
    double _value;

public:

    /// Construct with an angle in degrees
    explicit Degrees(double v): _value(v)
    {}

    /// Get the angle in degrees
    double asDegrees() const { return _value; }

    /// Get the angle in radians
    double asRadians() const { return degrees_to_radians(_value); }
};

/// Class representing an angle in radians
struct Radians
{
    double _value;

public:

    /// Construct with an angle in radians
    explicit Radians(double v): _value(v)
    {}

    /// Get the angle in degrees
    double asDegrees() const { return radians_to_degrees(_value); }

    /// Get the angle in radians
    double asRadians() const { return _value; }
};

}