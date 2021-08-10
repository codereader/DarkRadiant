#pragma once

#include <set>
#include "iselectiontest.h"
#include "math/Plane3.h"

namespace selection
{

namespace detail
{

class PlaneLess
{
public:
    bool operator()(const Plane3& plane, const Plane3& other) const
    {
        if (plane.normal().x() < other.normal().x()) {
            return true;
        }
        if (other.normal().x() < plane.normal().x()) {
            return false;
        }

        if (plane.normal().y() < other.normal().y()) {
            return true;
        }
        if (other.normal().y() < plane.normal().y()) {
            return false;
        }

        if (plane.normal().z() < other.normal().z()) {
            return true;
        }
        if (other.normal().z() < plane.normal().z()) {
            return false;
        }

        if (plane.dist() < other.dist()) {
            return true;
        }

        if (other.dist() < plane.dist()) {
            return false;
        }

        return false;
    }
};

}

class SelectedPlaneSet final : 
    public SelectedPlanes
{
private:
    typedef std::set<Plane3, detail::PlaneLess> PlaneSet;
    PlaneSet _selectedPlanes;

public:
    bool empty() const
    {
        return _selectedPlanes.empty();
    }

    void insert(const Plane3& plane)
    {
        _selectedPlanes.insert(plane);
    }

    bool contains(const Plane3& plane) const override
    {
        return _selectedPlanes.find(plane) != _selectedPlanes.end();
    }
};

}
