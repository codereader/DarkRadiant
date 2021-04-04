#pragma once

#include <istream>
#include "../StaticModelSurface.h"

namespace model
{

class AseModel
{
private:
    std::vector<StaticModelSurfacePtr> _surfaces;

public:
    StaticModelSurface& addSurface();

    const std::vector<StaticModelSurfacePtr>& getSurfaces() const;

    static std::shared_ptr<AseModel> CreateFromStream(std::istream& stream);
};

}
