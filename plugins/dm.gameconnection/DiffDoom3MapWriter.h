#pragma once

#include "imapformat.h"

#include <map>

namespace gameconn
{

class DiffStatus;

/**
 * Doom 3 map-patch writer for TheDarkMod hot reload / game connection feature.
 *
 * Creates plaintext representation of changes in .map file.
 */
class DiffDoom3MapWriter: public map::IMapWriter
{
    const std::map<std::string, DiffStatus> *_entityStatuses = nullptr;

    void writeEntityPreamble(const std::string &name, std::ostream& stream);
public:
    DiffDoom3MapWriter();
    void setStatuses(const std::map<std::string, DiffStatus> &entityStatuses);

    void beginWriteMap(const scene::IMapRootNodePtr& root, std::ostream& stream) override;
    void endWriteMap(const scene::IMapRootNodePtr& root, std::ostream& stream) override;

    // Entity export methods
    void beginWriteEntity(const IEntityNodePtr& entity, std::ostream& stream) override;
    void endWriteEntity(const IEntityNodePtr& entity, std::ostream& stream) override;
    void writeRemoveEntityStub(const std::string &name, std::ostream& stream);

    // Brush export methods
    void beginWriteBrush(const IBrushNodePtr& brush, std::ostream& stream) override;
    void endWriteBrush(const IBrushNodePtr& brush, std::ostream& stream) override;

    // Patch export methods
    void beginWritePatch(const IPatchNodePtr& patch, std::ostream& stream) override;
    void endWritePatch(const IPatchNodePtr& patch, std::ostream& stream) override;
};

}
