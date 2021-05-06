#pragma once

#include "math/Vector3.h"

namespace map
{

/// Parsed point positions from a .lin file
class PointTrace
{
    // List of point positions
    std::vector<Vector3> _points;

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

    /// Return number of points parsed
    std::size_t size() const { return _points.size(); }
};

}