#include "DarkmodTxt.h"

#include "i18n.h"
#include "iarchive.h"
#include "itextstream.h"
#include "ifilesystem.h"
#include "igame.h"
#include "os/fs.h"

#include <fstream>
#include <fmt/format.h>
#include "string/trim.h"
#include "string/convert.h"
#include "string/case_conv.h"

namespace map
{

const std::string& DarkmodTxt::getTitle()
{
	return _title;
}

void DarkmodTxt::setTitle(const std::string& title)
{
	_title = title;
}

const std::string& DarkmodTxt::getAuthor()
{
	return _author;
}

void DarkmodTxt::setAuthor(const std::string& author)
{
	_author = author;
}

const std::string& DarkmodTxt::getDescription()
{
	return _description;
}

void DarkmodTxt::setDescription(const std::string& desc)
{
	_description = desc;
}

const std::string& DarkmodTxt::getVersion()
{
	return _version;
}

void DarkmodTxt::setVersion(const std::string& version)
{
	_version = version;
}

const std::string& DarkmodTxt::getReqTdmVersion()
{
	return _reqTdmVersion;
}

void DarkmodTxt::setReqTdmVersion(const std::string& reqVersion)
{
	_reqTdmVersion = reqVersion;
}

const DarkmodTxt::TitleList& DarkmodTxt::getMissionTitles()
{
	return _missionTitles;
}

void DarkmodTxt::setMissionTitles(const DarkmodTxt::TitleList& list)
{
	_missionTitles = list;
}

void DarkmodTxt::ParseMissionTitles(std::vector<std::string>& titleList, const std::string& source)
{
	std::size_t titleNum = 1;
	std::size_t endIndex = 0;

	while (true)
	{
		std::string start = fmt::format("Mission {0:d} Title:", titleNum);
		std::string end = fmt::format("Mission {0:d} Title:", titleNum + 1);

		std::size_t startIndex = source.find(start, endIndex);

		if (startIndex == std::string::npos) break;

		endIndex = source.find(end, startIndex);

		// Extract next title string
		std::string title = source.substr(startIndex, (endIndex != std::string::npos) ? endIndex - startIndex : source.size() - startIndex);
		string::trim_left(title, start);
		string::trim(title);
		
		titleList.push_back(title);

		++titleNum;
	}
}

DarkmodTxtPtr DarkmodTxt::CreateFromString(const std::string& contents)
{
	DarkmodTxtPtr info(new DarkmodTxt);

	try
	{
		// Determine the positions in the file
		std::size_t titlePos = contents.find("Title:");
		std::size_t missionTitlesPos = contents.find("Mission 1 Title:");
		std::size_t descPos = contents.find("Description:");
		std::size_t authorPos = contents.find("Author:");
		std::size_t versionPos = contents.find("\nVersion:");
		std::size_t reqVersionPos = contents.find("Required TDM Version:");

		// Validate the order of the markers in the file
		bool positionsValid = titlePos != std::string::npos && titlePos < descPos && // Title is required & before description (or EOF)
			(missionTitlesPos == std::string::npos || missionTitlesPos < descPos) && // Optional Mission Titles & before description (or EOF)
			(descPos == std::string::npos || descPos < authorPos) &&				 // Optional description & before author (or EOF)
			(authorPos == std::string::npos || authorPos < versionPos) &&			 // Author optional & before description (or EOF)
			(versionPos == std::string::npos || versionPos < reqVersionPos);		 // Version optional & before req version (or EOF)

		if (!positionsValid)
		{
			throw ParseException(_("Order of the elements Title/Description/Author/etc. is incorrect"));
		}

		std::size_t len = contents.size();

		if (titlePos != std::string::npos)
		{
			std::size_t endPos = (missionTitlesPos != std::string::npos) ? missionTitlesPos : descPos;

			info->_title = contents.substr(titlePos, (endPos != std::string::npos) ? endPos - titlePos : len - titlePos);
			string::trim_left(info->_title, "Title:");
			string::trim(info->_title);
		}

		info->_missionTitles.clear();
		info->_missionTitles.push_back(info->_title); // [0] is title by default

		if (missionTitlesPos != std::string::npos)
		{
			std::string missionTitles = contents.substr(missionTitlesPos, (descPos != std::string::npos) ? descPos - missionTitlesPos: len - missionTitlesPos);
			ParseMissionTitles(info->_missionTitles, missionTitles);
		}

		if (descPos != std::string::npos)
		{
			info->_description = contents.substr(descPos, (authorPos != std::string::npos) ? authorPos - descPos : len - descPos);
			string::trim_left(info->_description, "Description:");
			string::trim(info->_description);
		}

		if (authorPos != std::string::npos)
		{
			std::size_t endPos = (versionPos != std::string::npos) ? versionPos : reqVersionPos;

			info->_author = contents.substr(authorPos, (endPos != std::string::npos) ? endPos - authorPos : len - authorPos);
			string::trim_left(info->_author, "Author:");
			string::trim(info->_author);
		}

		if (versionPos != std::string::npos)
		{
			info->_version = contents.substr(versionPos, (reqVersionPos != std::string::npos) ? reqVersionPos - versionPos : len - versionPos);
			string::trim_left(info->_version, "\nVersion:");
			string::trim(info->_version);
		}

		if (reqVersionPos != std::string::npos)
		{
			info->_reqTdmVersion = contents.substr(reqVersionPos, len - reqVersionPos);

			string::trim_left(info->_reqTdmVersion, "Required TDM Version:");
			string::trim_left(info->_reqTdmVersion, "v");
			string::trim(info->_reqTdmVersion);
		}
	}
	catch (const std::exception& ex)
	{
		// Convert ordinary exceptions in ParseExceptions
		rError() << "Exception parsing darkmod.txt: " << ex.what() << std::endl;
		throw ParseException(ex.what());
	}

	return info;
}

DarkmodTxtPtr DarkmodTxt::CreateFromStream(std::istream& stream)
{
	// Read all the stream contents into a string
	std::string str(std::istreambuf_iterator<char>(stream), {});
	return CreateFromString(str);
}

DarkmodTxtPtr DarkmodTxt::LoadForCurrentMod()
{
	std::string darkmodTxtPath = GetPathForCurrentMod();

	rMessage() << "Trying to open file " << darkmodTxtPath << std::endl;

	ArchiveTextFilePtr file = GlobalFileSystem().openTextFileInAbsolutePath(darkmodTxtPath);

	if (file)
	{
		std::istream stream(&(file->getInputStream()));
		return CreateFromStream(stream);
	}

	return std::make_shared<DarkmodTxt>();
}

std::string DarkmodTxt::toString()
{
	std::string output;

	if (!_title.empty())
	{
		output += fmt::format("Title: {0}", _title);
	}

	if (_missionTitles.size() > 1)
	{
		// Skip the first string, which is the same as the title
		for (std::size_t i = 1; i < _missionTitles.size(); ++i)
		{
			output += fmt::format("\nMission {1:d} Title: {0}", _missionTitles[i], i);
		}
	}

	if (!_description.empty())
	{
		output += fmt::format("\nDescription: {0}", _description);
	}

	if (!_author.empty())
	{
		output += fmt::format("\nAuthor: {0}", _author);
	}

	if (!_version.empty())
	{
		output += fmt::format("\nVersion: {0}", _version);
	}

	if (!_reqTdmVersion.empty())
	{
		output += fmt::format("\nRequired TDM Version: {0}", _reqTdmVersion);
	}

	return output;
}

void DarkmodTxt::saveToCurrentMod()
{
	std::string outputPath = GetPathForCurrentMod();

	rMessage() << "Writing darkmod.txt contents to " << outputPath << std::endl;

	std::ofstream outputStream;

	// Let the stream throw exceptions
	std::ios_base::iostate exceptionMask = outputStream.exceptions() | std::ios::failbit;
	outputStream.exceptions(exceptionMask);

	try
	{
		outputStream.open(outputPath);
		outputStream << toString();
		outputStream.close();

		rMessage() << "Successfully wrote darkmod.txt contents to " << outputPath << std::endl;
	}
	catch (std::ios_base::failure& ex)
	{
		throw std::runtime_error(fmt::format(_("Could not write darkmod.txt contents:\n{0}"), ex.what()));
	}
}

std::string DarkmodTxt::GetPathForCurrentMod()
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

	fs::path darkmodTxtPath = fs::path(modPath) / NAME();

	return darkmodTxtPath.string();
}

}
