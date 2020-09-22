#include "DiffDoom3MapWriter.h"
#include "DiffStatus.h"

#include "ientity.h"

namespace gameconn
{

DiffDoom3MapWriter::DiffDoom3MapWriter() {}

void DiffDoom3MapWriter::setStatuses(const std::map<std::string, DiffStatus> &entityStatuses) {
    _entityStatuses = &entityStatuses;
}

void DiffDoom3MapWriter::beginWriteMap(const scene::IMapRootNodePtr& root, std::ostream& stream) {}
void DiffDoom3MapWriter::endWriteMap(const scene::IMapRootNodePtr& root, std::ostream& stream) {}

void DiffDoom3MapWriter::writeEntityPreamble(const std::string &name, std::ostream& stream) {
    DiffStatus status = _entityStatuses->at(name);
    assert(status.isModified());
    const char *statusWord = "modify";
    if (status.needsRespawn())
        statusWord = "modify_respawn";
    if (status.isAdded())
        statusWord = "add";
    if (status.isRemoved())
        statusWord = "remove";
    stream << statusWord << " entity" << std::endl;
}

void DiffDoom3MapWriter::writeRemoveEntityStub(const std::string &name, std::ostream& stream) {
    writeEntityPreamble(name, stream);
    stream << "{" << std::endl;
    stream << "\"name\" \"" << name << "\"" << std::endl;
    stream << "}" << std::endl;
}

void DiffDoom3MapWriter::beginWriteEntity(const IEntityNodePtr& entity, std::ostream& stream) {
    const std::string &name = entity->name();
    writeEntityPreamble(name, stream);
    stream << "{" << std::endl;

    // Entity key values
    entity->getEntity().forEachKeyValue([&](const std::string& key, const std::string& value)
    {
        stream << "\"" << key << "\" \"" << value << "\"" << std::endl;
    });
}
void DiffDoom3MapWriter::endWriteEntity(const IEntityNodePtr& entity, std::ostream& stream) {
    stream << "}" << std::endl;
}

void DiffDoom3MapWriter::beginWriteBrush(const IBrushNodePtr&, std::ostream&)
{}

void DiffDoom3MapWriter::endWriteBrush(const IBrushNodePtr&, std::ostream&)
{}

void DiffDoom3MapWriter::beginWritePatch(const IPatchNodePtr&, std::ostream&)
{}

void DiffDoom3MapWriter::endWritePatch(const IPatchNodePtr&, std::ostream&)
{}

}
