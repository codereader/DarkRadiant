#pragma once

#include "map/format/Doom3MapWriter.h"

namespace map
{

class DiffStatus;

/**
 * Doom 3 map-patch writer for TheDarkMod hot reload / game connection feature.
 *
 * Creates plaintext representation of changes in .map file.
 */
class DiffDoom3MapWriter : public map::Doom3MapWriter
{
    const std::map<std::string, DiffStatus> *_entityStatuses = nullptr;

    void writeEntityPreamble(const std::string &name, std::ostream& stream);
public:
    DiffDoom3MapWriter();
    void setStatuses(const std::map<std::string, DiffStatus> &entityStatuses);

    virtual void beginWriteMap(const scene::IMapRootNodePtr& root, std::ostream& stream) override;
    virtual void endWriteMap(const scene::IMapRootNodePtr& root, std::ostream& stream) override;

    // Entity export methods
    virtual void beginWriteEntity(const IEntityNodePtr& entity, std::ostream& stream) override;
    virtual void endWriteEntity(const IEntityNodePtr& entity, std::ostream& stream) override;
    void writeRemoveEntityStub(const std::string &name, std::ostream& stream);

    /*// Brush export methods
    virtual void beginWriteBrush(const IBrushNodePtr& brush, std::ostream& stream) override;
    virtual void endWriteBrush(const IBrushNodePtr& brush, std::ostream& stream) override;

    // Patch export methods
    virtual void beginWritePatch(const IPatchNodePtr& patch, std::ostream& stream) override;
    virtual void endWritePatch(const IPatchNodePtr& patch, std::ostream& stream) override;*/
};

}
