#pragma once

#include "iarchive.h"
#include <functional>

class ArchiveVisitor :
	public Archive::Visitor
{
private:
	std::function<void(const std::string&)> _visitorFunc;
	Archive::EMode _mode;
	std::size_t _depth;

public:
	ArchiveVisitor(const std::function<void(const std::string&)>& func, Archive::EMode mode, std::size_t depth) :
		_visitorFunc(func),
		_mode(mode),
		_depth(depth)
	{}

	void visitFile(const std::string& name)
	{
		if ((_mode & Archive::eFiles) != 0)
		{
			_visitorFunc(name);
		}
	}

	bool visitDirectory(const std::string& name, std::size_t depth)
	{
		if ((_mode & Archive::eDirectories) != 0)
		{
			_visitorFunc(name);
		}

		if (depth == _depth)
		{
			return true;
		}

		return false;
	}
};
