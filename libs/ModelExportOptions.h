#pragma once

#include <stdexcept>
#include <string>
#include "math/Vector3.h"

namespace model
{

enum class ModelExportOrigin
{
    MapOrigin,
    SelectionCenter,
    EntityOrigin,
    CustomOrigin,
};

struct ModelExportOptions
{
    std::string outputFilename;		// full export path
    std::string outputFormat;		// model exporter extension
    bool skipCaulk;					// whether to skip caulk
    ModelExportOrigin exportOrigin; // where to center objects
    bool replaceSelectionWithModel;	// delete the selection and put the exported model in its place
    std::string entityName;			// when EntityOrigin is chosen, this defines the entity to pick
    bool exportLightsAsObjects;		// will export lights as small octahedrons
    Vector3 customExportOrigin;		// used when exportOrigin == ModelExportOrigin::CustomOrigin
};

inline std::string getExportOriginString(ModelExportOrigin option)
{
    switch (option)
    {
    case ModelExportOrigin::MapOrigin: return "MapOrigin";
    case ModelExportOrigin::SelectionCenter: return "SelectionCenter";
    case ModelExportOrigin::EntityOrigin: return "EntityOrigin";
    case ModelExportOrigin::CustomOrigin: return "CustomOrigin";
    default: throw std::invalid_argument("Unknown model export option");
    }
}

inline ModelExportOrigin getExportOriginFromString(const std::string& optionString)
{
    if (optionString == "MapOrigin") return ModelExportOrigin::MapOrigin;
    if (optionString == "SelectionCenter") return ModelExportOrigin::SelectionCenter;
    if (optionString == "EntityOrigin") return ModelExportOrigin::EntityOrigin;
    if (optionString == "CustomOrigin") return ModelExportOrigin::CustomOrigin;

    return ModelExportOrigin::MapOrigin;
}

}
