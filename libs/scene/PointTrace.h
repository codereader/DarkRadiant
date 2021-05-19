#pragma once

#include "math/Vector3.h"

namespace map
{

/// Parsed point positions from a .lin file
class PointTrace
{
public:
    using Points = std::vector<Vector3>;

private:

    // List of point positions
    Points _points;

public:

    /// Construct a PointTrace to read point data from the given stream
    explicit PointTrace(std::istream& stream)
    {
        // Point file consists of one point per line, with three components
        double x, y, z;
        while (stream >> x >> y >> z)
        {
            _points.push_back(Vector3(x, y, z));
        }
    }

    /// Return points parsed
    const Points& points() const { return _points; }
};

}