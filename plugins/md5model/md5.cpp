/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "MD5Surface.h"
#include "md5.h"
#include "MD5ModelLoader.h"

#include "imodel.h"
#include "archivelib.h"

#include "MD5Parser.h"

//bool MD5Anim_parse(Tokeniser& tokeniser)
//{
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseVersion(tokeniser));
//  tokeniser.nextLine();
//
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "commandline"));
//  const char* commandline;
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseString(tokeniser, commandline));
//  tokeniser.nextLine();
//
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "numFrames"));
//  std::size_t numFrames;
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseSize(tokeniser, numFrames));
//  tokeniser.nextLine();
//
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "numJoints"));
//  std::size_t numJoints;
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseSize(tokeniser, numJoints));
//  tokeniser.nextLine();
//
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "frameRate"));
//  std::size_t frameRate;
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseSize(tokeniser, frameRate));
//  tokeniser.nextLine();
//
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "numAnimatedComponents"));
//  std::size_t numAnimatedComponents;
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseSize(tokeniser, numAnimatedComponents));
//  tokeniser.nextLine();
//
//  // parse heirarchy
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "hierarchy"));
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "{"));
//  tokeniser.nextLine();
//
//  for(std::size_t i = 0; i < numJoints; ++i)
//  {
//    const char* name;
//    MD5_RETURN_FALSE_IF_FAIL(MD5_parseString(tokeniser, name));
//    int parent;
//    MD5_RETURN_FALSE_IF_FAIL(MD5_parseInteger(tokeniser, parent));
//    std::size_t flags;
//    MD5_RETURN_FALSE_IF_FAIL(MD5_parseSize(tokeniser, flags));
//    std::size_t index;
//    MD5_RETURN_FALSE_IF_FAIL(MD5_parseSize(tokeniser, index));
//    tokeniser.nextLine();
//  }
//
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "}"));
//  tokeniser.nextLine();
//
//  // parse bounds
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "bounds"));
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "{"));
//  tokeniser.nextLine();
//
//  for(std::size_t i = 0; i < numFrames; ++i)
//  {
//    Vector3 mins;
//    MD5_RETURN_FALSE_IF_FAIL(MD5_parseVector3(tokeniser, mins));
//    Vector3 maxs;
//    MD5_RETURN_FALSE_IF_FAIL(MD5_parseVector3(tokeniser, maxs));
//    tokeniser.nextLine();
//  }
//
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "}"));
//  tokeniser.nextLine();
//
//  // parse baseframe
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "baseframe"));
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "{"));
//  tokeniser.nextLine();
//
//  for(std::size_t i = 0; i < numJoints; ++i)
//  {
//    Vector3 position;
//    MD5_RETURN_FALSE_IF_FAIL(MD5_parseVector3(tokeniser, position));
//    Vector3 rotation;
//    MD5_RETURN_FALSE_IF_FAIL(MD5_parseVector3(tokeniser, rotation));
//    tokeniser.nextLine();
//  }
//
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "}"));
//  tokeniser.nextLine();
//
//  // parse frames
//  for(std::size_t i = 0; i < numFrames; ++i)
//  {
//    MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "frame"));
//    MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "{"));
//    tokeniser.nextLine();
//
//    for(std::size_t i = 0; i < numAnimatedComponents; ++i)
//    {
//      float component;
//      MD5_RETURN_FALSE_IF_FAIL(MD5_parseFloat(tokeniser, component));
//      tokeniser.nextLine();
//    }
//
//    MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "}"));
//    tokeniser.nextLine();
//  }
//
//  return true;
//}

namespace md5 {

scene::INodePtr loadMD5Model(ArchiveFile& file) {
	// greebo: Get the Inputstream from the given file
	BinaryToTextInputStream<InputStream> inputStream(file.getInputStream());
	
	// Construct a new Node
	MD5ModelNodePtr modelNode(new MD5ModelNode);
	
	// Construct a DefTokeniser and start parsing
	try {
		std::istream is(&inputStream);
		MD5Parser parser(is);
		
		// Invoke the parser routine
		parser.parseToModel(modelNode->model());
	}
	catch (parser::ParseException e) {
		globalErrorStream() << "[md5model] Parse failure. Exception was:\n"
							<< e.what() << "\n";		
	}
	
	// Upcast the MD5ModelNode to scene::INode and return
	return modelNode;
}

} // namespace md5
