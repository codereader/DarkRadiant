#include "MD5Anim.h"

#include "itextstream.h"
#include "string/string.h"

namespace md5
{

MD5Anim::MD5Anim() :
	_numFrames(-1),
	_frameRate(0),
	_numAnimatedComponents(-1)
{}

void MD5Anim::parseJointHierarchy(parser::DefTokeniser& tok)
{
	tok.assertNextToken("hierarchy");
	tok.assertNextToken("{");

	for (std::size_t i = 0; i < _joints.size(); ++i)
	{
		// Assign the correct joint ID
		_joints[i].id = static_cast<int>(i);

		// Syntax: "<jointName>"	<parentId> <animComponentMask> <firstKey>
		_joints[i].name = tok.nextToken();

		int parentId = strToInt(tok.nextToken());
		_joints[i].parentId = parentId;	

		_joints[i].animComponents = strToSizet(tok.nextToken());
		_joints[i].firstKey = strToSizet(tok.nextToken());

		// Some sanity checks
		assert(_joints[i].parentId == -1 || (_joints[i].parentId >= 0 && _joints[i].parentId < _joints.size()));
		assert(_joints[i].animComponents < Joint::INVALID_COMPONENT);

		// Add this joint as child to its parent joint
		if (parentId >= 0)
		{
			_joints[parentId].children.push_back(_joints[i].id);
		}
	}

	// If we don't hit a closing brace at this point something went amiss
	tok.assertNextToken("}");
}

void MD5Anim::parseFrameBounds(parser::DefTokeniser& tok)
{
	tok.assertNextToken("bounds");
	tok.assertNextToken("{");
		
	for (std::size_t i = 0; i < _numFrames; ++i)
	{
		tok.assertNextToken("(");

		_bounds[i].origin.x() = strToFloat(tok.nextToken());
		_bounds[i].origin.y() = strToFloat(tok.nextToken());
		_bounds[i].origin.z() = strToFloat(tok.nextToken());

		tok.assertNextToken(")");

		tok.assertNextToken("(");

		_bounds[i].extents.x() = strToFloat(tok.nextToken());
		_bounds[i].extents.y() = strToFloat(tok.nextToken());
		_bounds[i].extents.z() = strToFloat(tok.nextToken());

		tok.assertNextToken(")");
	}

	tok.assertNextToken("}");
}

void MD5Anim::parseBaseFrame(parser::DefTokeniser& tok)
{
	tok.assertNextToken("baseframe");
	tok.assertNextToken("{");

	for (std::size_t i = 0; i < _joints.size(); ++i)
	{
		tok.assertNextToken("(");
		
		_baseFrame[i].origin.x() = strToFloat(tok.nextToken());
		_baseFrame[i].origin.y() = strToFloat(tok.nextToken());
		_baseFrame[i].origin.z() = strToFloat(tok.nextToken());

		tok.assertNextToken(")");

		tok.assertNextToken("(");

		Vector3 rawRotation;
		rawRotation.x() = strToFloat(tok.nextToken());
		rawRotation.y() = strToFloat(tok.nextToken());
		rawRotation.z() = strToFloat(tok.nextToken());

		// Calculate the fourth component of the quaternion
		float lSq = rawRotation.getLengthSquared();
	    float w = -sqrt(1.0f - lSq);

	    if (isNaN(w))
		{
	    	w = 0;
	    }

		_baseFrame[i].orientation = Quaternion(rawRotation, w);

		tok.assertNextToken(")");
	}

	tok.assertNextToken("}");
}

void MD5Anim::parseFrame(std::size_t frame, parser::DefTokeniser& tok)
{
	tok.assertNextToken("frame");

	std::size_t parsedFrameNum = strToSizet(tok.nextToken());

	assert(frame == parsedFrameNum);

	tok.assertNextToken("{");

	// Reserve space for the floats
	_frames[parsedFrameNum].resize(_numAnimatedComponents);

	// Each frame block has <numAnimatedComponents> float values
	for (std::size_t i = 0; i < _numAnimatedComponents; ++i)
	{
		_frames[parsedFrameNum][i] = strToFloat(tok.nextToken());
	}

	tok.assertNextToken("}");
}

void MD5Anim::parseFromStream(std::istream& stream)
{
	parser::BasicDefTokeniser<std::istream> tokeniser(stream);
	parseFromTokens(tokeniser);
}

void MD5Anim::parseFromTokens(parser::DefTokeniser& tok)
{
	try
	{
		tok.assertNextToken("MD5Version");

		int version = strToInt(tok.nextToken());

		if (version != 10)
		{
			globalWarningStream() << "Unexpected version encountered: " << version 
				<< " (expected 10), will attempt to load anyway." << std::endl;
		}

		tok.assertNextToken("commandline");
		_commandLine = tok.nextToken();

		tok.assertNextToken("numFrames");
		_numFrames = strToInt(tok.nextToken());

		tok.assertNextToken("numJoints");
		std::size_t numJoints = strToSizet(tok.nextToken());

		// Adjust the arrays
		_joints.resize(numJoints);
		_bounds.resize(_numFrames);
		_baseFrame.resize(numJoints);
		_frames.resize(_numFrames);

		tok.assertNextToken("frameRate");
		_frameRate = strToInt(tok.nextToken());

		tok.assertNextToken("numAnimatedComponents");
		_numAnimatedComponents = strToInt(tok.nextToken());

		// Parse hierarchy block
		parseJointHierarchy(tok);
		
		// Parse bounds block
		parseFrameBounds(tok);

		// Parse base frame
		parseBaseFrame(tok);

		// Parse each actual frame
		for (std::size_t i = 0; i < _numFrames; ++i)
		{
			parseFrame(i, tok);
		}
	}
	catch (parser::ParseException& ex)
	{
		globalErrorStream() << "Error parsing MD5 Animation: " << ex.what() << std::endl;
	}
}

} // namespace
