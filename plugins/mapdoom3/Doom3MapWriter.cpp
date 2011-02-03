#include "Doom3MapWriter.h"

#include "igame.h"
#include "ientity.h"
#include "Tokens.h"

#include "primitivewriters/BrushDef3Exporter.h"
#include "primitivewriters/PatchDefExporter.h"

namespace map
{

namespace
{
	const char* const RKEY_GAME_MAP_VERSION = "/mapFormat/version";
}

Doom3MapWriter::Doom3MapWriter() :
	_entityCount(0),
	_primitiveCount(0)
{}

void Doom3MapWriter::beginWriteMap(std::ostream& stream)
{
	// Get the map version from the game def
	game::IGamePtr curGame = GlobalGameManager().currentGame();
	assert(curGame != NULL);

	xml::NodeList nodes = curGame->getLocalXPath(RKEY_GAME_MAP_VERSION);
	assert(!nodes.empty());

	std::string mapVersion = nodes[0].getAttributeValue("value");

	// Write the version tag
    stream << VERSION << " " << mapVersion << std::endl;
}

void Doom3MapWriter::endWriteMap(std::ostream& stream)
{
	// nothing
}

void Doom3MapWriter::beginWriteEntity(const Entity& entity, std::ostream& stream)
{
	// Write out the entity number comment
	stream << "// entity " << _entityCount++ << std::endl;

	// Entity opening brace
	stream << "{" << std::endl;

	// Entity key values
	writeEntityKeyValues(entity, stream);
}

void Doom3MapWriter::writeEntityKeyValues(const Entity& entity, std::ostream& stream)
{
	// Create a local Entity visitor class to export the keyvalues
	// to the output stream
	class WriteKeyValue : 
		public Entity::Visitor
	{
	private:
		// Stream to write to
		std::ostream& _os;
	public:
		// Constructor
		WriteKeyValue(std::ostream& os) : 
			_os(os)
	    {}

		// Required visit function
    	void visit(const std::string& key, const std::string& value)
		{
			_os << "\"" << key << "\" \"" << value << "\"" << std::endl;
		}

	} visitor(stream);

	// Visit the entity
	entity.forEachKeyValue(visitor);
}

void Doom3MapWriter::endWriteEntity(const Entity& entity, std::ostream& stream)
{
	// Write the closing brace for the entity
	stream << "}" << std::endl;
}

void Doom3MapWriter::beginWriteBrush(const IBrush& brush, std::ostream& stream)
{
	// Primitive count comment
	stream << "// primitive " << _primitiveCount++ << std::endl;

	// Export brushDef3 definition to stream
	BrushDef3Exporter::exportBrush(stream, brush);
}

void Doom3MapWriter::endWriteBrush(const IBrush& brush, std::ostream& stream)
{
	// nothing
}

void Doom3MapWriter::beginWritePatch(const IPatch& patch, std::ostream& stream)
{
	// Primitive count comment
	stream << "// primitive " << _primitiveCount++ << std::endl;

	// Export patch here _mapStream
	PatchDefExporter::exportPatch(stream, patch);
}

void Doom3MapWriter::endWritePatch(const IPatch& patch, std::ostream& stream)
{
	// nothing
}

} // namespace
