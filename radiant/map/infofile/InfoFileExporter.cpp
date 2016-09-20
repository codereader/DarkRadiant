#include "InfoFileExporter.h"

#include "imapinfofile.h"
#include "itextstream.h"
#include "InfoFile.h"

namespace map
{

InfoFileExporter::InfoFileExporter(std::ostream& stream) :
    _stream(stream)
{
	GlobalMapInfoFileManager().foreachModule([](IMapInfoFileModule& module)
	{
		module.onInfoFileSaveStart();
	});

    // Write the information file header
    _stream << InfoFile::HEADER_SEQUENCE << " " << InfoFile::MAP_INFO_VERSION << std::endl;
    _stream << "{" << std::endl;
}

InfoFileExporter::~InfoFileExporter()
{
	// Tell the info file modules to write their data now
	GlobalMapInfoFileManager().foreachModule([&](IMapInfoFileModule& module)
	{
		rMessage() << "Writing info file blocks for " << module.getName() << std::endl;

		module.writeBlocks(_stream);
	});

	// Write the closing braces of the information file
    _stream << "}" << std::endl;

	_stream.flush();

	GlobalMapInfoFileManager().foreachModule([](IMapInfoFileModule& module)
	{
		module.onInfoFileSaveFinished();
	});
}

void InfoFileExporter::visitEntity(const scene::INodePtr& node, std::size_t entityNum)
{
	GlobalMapInfoFileManager().foreachModule([&](IMapInfoFileModule& module)
	{
		module.onSaveEntity(node, entityNum);
	});
}

void InfoFileExporter::visitPrimitive(const scene::INodePtr& node, std::size_t entityNum, std::size_t primitiveNum)
{
	GlobalMapInfoFileManager().foreachModule([&](IMapInfoFileModule& module)
	{
		module.onSavePrimitive(node, entityNum, primitiveNum);
	});
}



} // namespace
