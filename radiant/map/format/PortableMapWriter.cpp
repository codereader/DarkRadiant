#include "PortableMapWriter.h"

#include "igame.h"
#include "ientity.h"

#include "string/string.h"

namespace map
{

PortableMapWriter::PortableMapWriter() :
	_entityCount(0),
	_primitiveCount(0),
	_document(xml::Document::create()),
	_map(_document.addTopLevelNode("map")),
	_curEntityPrimitives(nullptr)
{}

void PortableMapWriter::beginWriteMap(std::ostream& stream)
{}

void PortableMapWriter::endWriteMap(std::ostream& stream)
{
	stream << _document.saveToString();
}

void PortableMapWriter::beginWriteEntity(const IEntityNodePtr& entity, std::ostream& stream)
{
	auto node = _map.createChild("entity");
	node.setAttributeValue("number", string::to_string(_entityCount++));

	auto primitiveNode = node.createChild("primitives");
	_curEntityPrimitives = xml::Node(primitiveNode.getNodePtr());

	auto keyValues = node.createChild("keyValues");

	// Export the entity key values
	entity->getEntity().forEachKeyValue([&](const std::string& key, const std::string& value)
	{
		auto kv = keyValues.createChild("keyValue");
		kv.setAttributeValue("key", key);
		kv.setAttributeValue("value", value);
	});
}

void PortableMapWriter::endWriteEntity(const IEntityNodePtr& entity, std::ostream& stream)
{
	// Reset the primitive count again
	_primitiveCount = 0;

	_curEntityPrimitives = xml::Node(nullptr);
}

void PortableMapWriter::beginWriteBrush(const IBrushNodePtr& brush, std::ostream& stream)
{
	assert(_curEntityPrimitives.getNodePtr() != nullptr);

	auto node = _curEntityPrimitives.createChild("brush");
	node.setAttributeValue("number", string::to_string(_primitiveCount++));
}

void PortableMapWriter::endWriteBrush(const IBrushNodePtr& brush, std::ostream& stream)
{
	// nothing
}

void PortableMapWriter::beginWritePatch(const IPatchNodePtr& patch, std::ostream& stream)
{
	assert(_curEntityPrimitives.getNodePtr() != nullptr);

	auto node = _curEntityPrimitives.createChild("patch");
	node.setAttributeValue("number", string::to_string(_primitiveCount++));
}

void PortableMapWriter::endWritePatch(const IPatchNodePtr& patch, std::ostream& stream)
{
	// nothing
}

} // namespace
