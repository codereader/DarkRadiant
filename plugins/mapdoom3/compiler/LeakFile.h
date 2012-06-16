#pragma once

#include "ientity.h"
#include "itextstream.h"
#include "BspTree.h"
#include "ProcFile.h"

#include "string/convert.h"
#include <fstream>
#include <boost/format.hpp>

namespace map
{

class LeakFile
{
private:
	std::vector<Vector3> _line;

public:
	LeakFile(const BspTree& tree)
	{
		if (!tree.outside->occupied)
		{
			return;
		}

		BspTreeNodePtr node = tree.outside;

		while (node->occupied > 1)
		{
			// find the best portal exit
			std::size_t next = node->occupied;

			std::size_t s = 0;
			BspTreeNodePtr nextNode;
			ProcPortalPtr nextPortal;

			for (ProcPortalPtr p = node->portals; p ; p = p->next[1-s])
			{
				s = (p->nodes[0] == node) ? 1 : 0;

				if (p->nodes[s]->occupied && p->nodes[s]->occupied < next)
				{
					nextPortal = p;
					nextNode = p->nodes[s];
					next = nextNode->occupied;
				}
			}

			node = nextNode;

			_line.push_back(nextPortal->winding.getCenter());
		}

		// add the occupant's center
		Vector3 mid = string::convert<Vector3>(
            node->occupant->mapEntity->getEntity().getKeyValue("origin")
        );

		_line.push_back(mid);
	}

	void writeToFile(const std::string& leakFilename)
	{
		rMessage() << "Writing leakfile to: " << leakFilename << std::endl;

		std::ofstream stream(leakFilename.c_str());

		if (!stream.fail())
		{
			for (std::vector<Vector3>::const_iterator i = _line.begin(); i != _line.end(); ++i)
			{
				stream << (boost::format("%f %f %f") % i->x() % i->y() % i->z()) << std::endl;
			}

			stream.flush();
			stream.close();
		}
		else
		{
			rError() << "Couldn't open leakfile." << std::endl;
		}
	}

	static const char* const Extension()
	{
		return ".lin";
	}
};
typedef boost::shared_ptr<LeakFile> LeakFilePtr;

} // namespace
