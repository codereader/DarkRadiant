#include "DarkmodTxt.h"

#include "i18n.h"
#include "iarchive.h"
#include "itextstream.h"
#include "ifilesystem.h"
#include "igame.h"
#include "os/fs.h"

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

const std::string& DarkmodTxt::getAuthor()
{
	return _author;
}

const std::string& DarkmodTxt::getDescription()
{
	return _description;
}

const std::string& DarkmodTxt::getVersion()
{
	return _version;
}

const std::string& DarkmodTxt::getReqTdmVersion()
{
	return _reqTdmVersion;
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
	std::string modPath = GlobalGameManager().getModPath();

	if (modPath.empty())
	{
		rMessage() << "Mod path empty, falling back to mod base path..." << std::endl;
		modPath = GlobalGameManager().getModBasePath();
	}

	fs::path darkmodTxtPath = fs::path(modPath) / NAME();

	rMessage() << "Trying to open file " << darkmodTxtPath << std::endl;

	ArchiveTextFilePtr file = GlobalFileSystem().openTextFileInAbsolutePath(darkmodTxtPath.string());

	if (file)
	{
		std::istream stream(&(file->getInputStream()));
		return CreateFromStream(stream);
	}

	return std::make_shared<DarkmodTxt>();
}

}
