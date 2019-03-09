#pragma once

#include "Archive.h"
#include "string/string.h"
#include "os/path.h"

#include <map>
#include <iostream>

namespace archive
{

namespace
{

// Returns the depth of a given path string, basically counting
// the number of parts in between forward slashes
// "" => 0
// "dds/" => 1
// "dds/textures/" => 2
// "dds/textures/darkmod/" => 3
// "dds/textures/darkmod/wood/" => 4
// "dds/textures/darkmod/wood/boards/" => 5
// "dds/textures/darkmod/wood/boards/dark_rough.dds" => 6
inline unsigned int getPathDepth(const char* path)
{
	unsigned int depth = 0;

	while (path != 0 && path[0] != '\0')
	{
		path = strchr(path, '/');

		if (path != 0)
		{
			++path;
		}

		++depth;
	}

	return depth;
}

}

/// \brief A generic unix-style file-system which maps paths to files and directories.
/// Provides average O(log n) find and insert methods.
/// \param file_type The data type which represents a file.
template<typename RecordType>
class GenericFileSystem
{
	class Path
	{
	private:
		std::string _path;
		unsigned int _depth;
	public:
		Path(const std::string& path) :
			_path(path),
			_depth(getPathDepth(_path.c_str()))
		{}

		Path(const char* start, std::size_t length) :
			Path(std::string(start, length))
		{}

		bool operator<(const Path& other) const
		{
			return string_less_nocase(c_str(), other.c_str());
		}

		unsigned int depth() const
		{
			return _depth;
		}

		const char* c_str() const
		{
			return _path.c_str();
		}

		const std::string& string() const
		{
			return _path;
		}
	};

public:
	class Entry
	{
		std::shared_ptr<RecordType> _record;
	public:
		Entry()
		{}

		Entry(const std::shared_ptr<RecordType>& record) :
			_record(record)
		{}

		std::shared_ptr<RecordType>& getRecord()
		{
			return _record;
		}

		bool isDirectory() const
		{
			return !_record;
		}
	};

private:
	typedef std::map<Path, Entry> Entries;
	Entries _entries;

public:
	typedef typename Entries::iterator iterator;
	typedef typename Entries::value_type value_type;
	typedef Entry entry_type;

	iterator begin()
	{
		return _entries.begin();
	}

	iterator end()
	{
		return _entries.end();
	}

	void clear()
	{
		_entries.clear();
	}

	/// \brief Returns the file at \p path.
	/// Creates all directories below \p path if they do not exist.
	/// O(log n) on average.
	entry_type& operator[](const Path& path)
	{
		const char* start = path.c_str();
		const char* end = path_remove_directory(path.c_str());

		while (end[0] != '\0')
		{
			// greebo: Take the substring from start to end
			Path dir(start, end - start);

			// And insert it as directory
			_entries.insert(value_type(dir, Entry()));

			end = path_remove_directory(end);
		}

		return _entries[path];
	}

	/// \brief Returns the file at \p path or end() if not found.
	iterator find(const Path& path)
	{
		return _entries.find(path);
	}

	/// \brief Performs a depth-first traversal of the file-system subtree rooted at \p root.
	/// Traverses the entire tree if \p root is "".
	/// Calls \p visitor.file() with the path to each file relative to the filesystem root.
	/// Calls \p visitor.directory() with the path to each directory relative to the filesystem root.
	void traverse(Archive::Visitor& visitor, const std::string& root)
	{
		unsigned int start_depth = getPathDepth(root.c_str());
		unsigned int skip_depth = 0;
		
		for (iterator i = begin(root); i != end() && i->first.depth() > start_depth; ++i)
		{
			if (i->first.depth() == skip_depth)
			{
				skip_depth = 0;
			}

			if (skip_depth == 0)
			{
				if (!i->second.isDirectory())
				{
					visitor.visitFile(i->first.string());
				}
				else if (visitor.visitDirectory(i->first.string(), i->first.depth() - start_depth))
				{
					skip_depth = i->first.depth();
				}
			}
		}
	}

private:
	iterator begin(const std::string& root)
	{
		if (root.empty())
		{
			return _entries.begin();
		}

		iterator i = _entries.find(root);

		if (i == _entries.end())
		{
			return i;
		}

		return ++i;
	}
};

}
