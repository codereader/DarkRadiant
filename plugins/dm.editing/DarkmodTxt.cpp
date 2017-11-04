#include "DarkmodTxt.h"

#include "iarchive.h"
#include "itextstream.h"
#include "ifilesystem.h"
#include "igame.h"
#include "os/fs.h"
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

const std::string& DarkmodTxt::getReqTdmVersion()
{
	return _reqTdmVersion;
}

void DarkmodTxt::ParseMissionTitles(std::vector<std::string>& titleList, const std::string& source)
{
	// TODO
}

DarkmodTxtPtr DarkmodTxt::CreateFromString(const std::string& contents)
{
	DarkmodTxtPtr info(new DarkmodTxt);

	// The positions need to be searched case-insensitively
	std::string contentsLower = string::to_lower_copy(contents);
	std::size_t titlePos = contentsLower.find("title:");
	std::size_t descPos = contentsLower.find("description:");
	std::size_t authorPos = contentsLower.find("author:");
	std::size_t versionPos = contentsLower.find("required tdm version:");
	std::size_t missionTitlesPos = contentsLower.find("mission 1 title:");

	std::size_t len = contents.size();

	if (titlePos != std::string::npos)
	{
		info->_title = contents.substr(titlePos, (missionTitlesPos != std::string::npos) ? 
			descPos - missionTitlesPos : (descPos != std::string::npos) ? descPos - titlePos : len - titlePos);
		string::trim_left(info->_title, "Title:");
		string::trim(info->_title);
	}

	info->_missionTitles.clear();
	info->_missionTitles.push_back(info->_title); // [0] is the title

	if (missionTitlesPos != std::string::npos)
	{
		std::string missionTitles = contents.substr(missionTitlesPos, (descPos != std::string::npos) ? missionTitlesPos - descPos : len - missionTitlesPos);
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
		info->_author = contents.substr(authorPos, (versionPos != std::string::npos) ? versionPos - authorPos : len - authorPos);
		string::trim_left(info->_author, "Author:");
		string::trim(info->_author);
	}

	if (versionPos != std::string::npos)
	{
		info->_reqTdmVersion = contents.substr(versionPos, len - versionPos);

		string::trim_left(info->_reqTdmVersion, "Required TDM Version:");
		string::trim_left(info->_reqTdmVersion, "v");
		string::trim(info->_reqTdmVersion);
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
