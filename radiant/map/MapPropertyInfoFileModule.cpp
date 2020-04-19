#include "MapPropertyInfoFileModule.h"

#include "itextstream.h"
#include "parser/DefTokeniser.h"
#include "string/replace.h"

namespace map
{

namespace
{
	const char* const MAP_PROPERTIES = "MapProperties";
	const char* const KEY_VALUE = "KeyValue";
}

std::string MapPropertyInfoFileModule::getName()
{
	return "Map Properties";
}

void MapPropertyInfoFileModule::onInfoFileSaveStart()
{}

void MapPropertyInfoFileModule::onBeginSaveMap(const scene::IMapRootNodePtr& root)
{
	// Load all the properties from the map root into a local store
	root->foreachProperty([this](const std::string& key, const std::string& value)
	{
		_store.setProperty(key, value);
	});
}

void MapPropertyInfoFileModule::onFinishSaveMap(const scene::IMapRootNodePtr& root)
{}

void MapPropertyInfoFileModule::onSavePrimitive(const scene::INodePtr& node, std::size_t entityNum, std::size_t primitiveNum)
{}

void MapPropertyInfoFileModule::onSaveEntity(const scene::INodePtr& node, std::size_t entityNum)
{}

void MapPropertyInfoFileModule::writeBlocks(std::ostream& stream)
{
	// Block output
	stream << "\t" << MAP_PROPERTIES << std::endl;

	stream << "\t{" << std::endl;

	_store.foreachProperty([&](const std::string& key, const std::string& value)
	{
		stream << "\t\t" << KEY_VALUE << " { " <<
			"\"" << string::replace_all_copy(key, "\"", "&quot;") << "\"" <<
			" " << 
			"\"" << string::replace_all_copy(value, "\"", "&quot;") << "\"" <<
			" } " << std::endl;
	});

	stream << "\t}" << std::endl;

	rMessage() << "Map Properties written." << std::endl;
}

void MapPropertyInfoFileModule::onInfoFileSaveFinished()
{
	_store.clearProperties();
}

void MapPropertyInfoFileModule::onInfoFileLoadStart()
{
	_store.clearProperties();
}

bool MapPropertyInfoFileModule::canParseBlock(const std::string& blockName)
{
	return blockName == MAP_PROPERTIES;
}

void MapPropertyInfoFileModule::parseBlock(const std::string& blockName, parser::DefTokeniser& tok)
{
	// The opening brace
	tok.assertNextToken("{");

	int blockLevel = 1;

	while (tok.hasMoreTokens() && blockLevel > 0)
	{
		std::string token = tok.nextToken();

		if (token == KEY_VALUE)
		{
			// KeyValue { "Key" "Value" }
			tok.assertNextToken("{");

			std::string key = tok.nextToken();
			std::string value = tok.nextToken();

			string::replace_all(key, "&quot;", "\"");
			string::replace_all(value, "&quot;", "\"");

			_store.setProperty(key, value);

			tok.assertNextToken("}");
		}
		else if (token == "{")
		{
			blockLevel++;
		}
		else if (token == "}")
		{
			blockLevel--;
		}
	}
}

void MapPropertyInfoFileModule::applyInfoToScene(const scene::IMapRootNodePtr& root, const NodeIndexMap& nodeMap)
{
	_store.foreachProperty([&](const std::string& key, const std::string& value)
	{
		root->setProperty(key, value);
	});
}

void MapPropertyInfoFileModule::onInfoFileLoadFinished()
{
	rMessage() << "[InfoFile]: Parsed " << _store.getPropertyCount() << " map properties." << std::endl;
}

}
