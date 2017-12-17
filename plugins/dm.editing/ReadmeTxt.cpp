#include "ReadmeTxt.h"

#include "i18n.h"
#include "iarchive.h"
#include "itextstream.h"
#include "ifilesystem.h"

namespace map
{

std::string ReadmeTxt::getFilename()
{
	return NAME();
}

const std::string& ReadmeTxt::getContents()
{
	return _contents;
}

void ReadmeTxt::setContents(const std::string& contents)
{
	_contents = contents;
}

ReadmeTxtPtr ReadmeTxt::CreateFromString(const std::string& contents)
{
	ReadmeTxtPtr info(new ReadmeTxt);

	// The readme.txt parser appears to be rather simple
	info->_contents = contents;

	return info;
}

ReadmeTxtPtr ReadmeTxt::CreateFromStream(std::istream& stream)
{
	// Read all the stream contents into a string
	std::string str(std::istreambuf_iterator<char>(stream), {});
	return CreateFromString(str);
}

ReadmeTxtPtr ReadmeTxt::LoadForCurrentMod()
{
	std::string readmeTxtPath = GetOutputPathForCurrentMod() + NAME();

	rMessage() << "Trying to open file " << readmeTxtPath << std::endl;

	ArchiveTextFilePtr file = GlobalFileSystem().openTextFileInAbsolutePath(readmeTxtPath);

	if (file)
	{
		std::istream stream(&(file->getInputStream()));
		return CreateFromStream(stream);
	}

	return std::make_shared<ReadmeTxt>();
}

std::string ReadmeTxt::toString()
{
	return _contents;
}

}
