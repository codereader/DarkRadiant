#include "MissionInfoTextFile.h"

#include <stdexcept>
#include <fstream>
#include <fmt/format.h>

#include "i18n.h"
#include "itextstream.h"
#include "igame.h"

#include "os/path.h"

namespace map
{

std::string MissionInfoTextFile::getFullOutputPath()
{
	return GetOutputPathForCurrentMod() + getFilename();
}

void MissionInfoTextFile::saveToCurrentMod()
{
	std::string outputPath = getFullOutputPath();

	rMessage() << "Writing " << getFilename() << " contents to " << outputPath << std::endl;

	std::ofstream outputStream;

	// Let the stream throw exceptions
	std::ios_base::iostate exceptionMask = outputStream.exceptions() | std::ios::failbit;
	outputStream.exceptions(exceptionMask);

	try
	{
		outputStream.open(outputPath);
		outputStream << toString();
		outputStream.close();

		rMessage() << "Successfully wrote " << getFilename() << " contents to " << outputPath << std::endl;
	}
	catch (std::ios_base::failure& ex)
	{
		throw std::runtime_error(fmt::format(_("Could not write {0} contents:\n{1}"), getFilename(), ex.what()));
	}
}

std::string MissionInfoTextFile::GetOutputPathForCurrentMod()
{
	std::string modPath = GlobalGameManager().getModPath();

	if (modPath.empty())
	{
		rMessage() << "Mod path empty, falling back to mod base path..." << std::endl;
		modPath = GlobalGameManager().getModBasePath();

		if (modPath.empty())
		{
			rMessage() << "Mod base path empty as well, falling back to user engine path..." << std::endl;
			modPath = GlobalGameManager().getUserEnginePath();
		}
	}

	return os::standardPathWithSlash(modPath);
}

}
