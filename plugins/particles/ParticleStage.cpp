#include "ParticleStage.h"

#include "itextstream.h"
#include <boost/lexical_cast.hpp>

namespace particles
{

namespace
{
	// Returns next token as number, or returns zero on fail, printing the given error message
	template<typename Type>
	inline Type parseWithErrorMsg(parser::DefTokeniser& tok, const char* errorMsg)
	{
		std::string str = tok.nextToken();

		try
		{
			return boost::lexical_cast<Type>(str);
		}
		catch (boost::bad_lexical_cast&)
		{
			globalErrorStream() << "[particles] " << errorMsg << ", token is '" << 
				str << "'" << std::endl;
			return 0;
		}
	}
}

ParticleStage::ParticleStage()
{
	reset();
}

ParticleStage::ParticleStage(parser::DefTokeniser& tok)
{
	// Parse from the tokens, but don't allow any parse exceptions
	// from leaving this destructor.
	try
	{
		parseFromTokens(tok);
	}
	catch (parser::ParseException& p)
	{
		globalErrorStream() << "[particles]: Could not parse particle stage: " << 
			p.what() << std::endl;
	}
	catch (boost::bad_lexical_cast& e)
	{
		globalErrorStream() << "[particles]: Invalid cast when parsing particle stage: " << 
			e.what() << std::endl;
	}
}

void ParticleStage::reset()
{
	_count = 1;
	_material.clear();

	_duration = 1;
	_cycles = 0;
	_bunching = 0.0f;

	_timeOffset = 0;
	_deadTime = 0;

	recalculateCycleMsec();

	_colour = Vector4(1,1,1,1);
	_fadeColour = Vector4(1,1,1,0);

	_fadeInFraction = 0.0f;
	_fadeOutFraction = 0.0f;
	_fadeIndexFraction = 0.0f;

	_animationFrames = 0;
	_animationRate = 0;

	_initialAngle = 0;

	_boundsExpansion = 0;

	_randomDistribution = true;
	_entityColor = false;

	_gravity = -1.0f;
	_applyWorldGravity = true;

	_orientationType = ORIENTATION_VIEW;
	_orientationParms[0] = _orientationParms[1] = _orientationParms[2] = _orientationParms[3] = 0;

	_distributionType = DISTRIBUTION_RECT;
	_distributionParms[0] = _distributionParms[1] = _distributionParms[2] = _distributionParms[3] = 0;

	_directionType = DIRECTION_CONE;
	_directionParms[0] = _directionParms[1] = _directionParms[2] = _directionParms[3] = 0;

	_customPathType = PATH_STANDARD;
	_customPathParms[0] = _customPathParms[1] = _customPathParms[2] = _customPathParms[3] = 0;
	_customPathParms[4] = _customPathParms[5] = _customPathParms[6] = _customPathParms[7] = 0;

	_speed = ParticleParameter();
	_rotationSpeed = ParticleParameter();
}

void ParticleStage::parseFromTokens(parser::DefTokeniser& tok)
{
	reset();

	std::string token = tok.nextToken();

	while (token != "}")
	{
		if (token == "count")
		{
			setCount(parseWithErrorMsg<int>(tok, "Bad count value"));
		}
		else if (token == "material")
		{
			setMaterialName(tok.nextToken());
		}
		else if (token == "time") // duration
		{
			setDuration(parseWithErrorMsg<float>(tok, "Bad duration value"));
		}
		else if (token == "cycles")
		{
			setCycles(parseWithErrorMsg<float>(tok, "Bad cycles value"));
		}
		else if (token == "timeOffset")
		{
			setTimeOffset(parseWithErrorMsg<float>(tok, "Bad time offset value"));
		}
		else if (token == "deadTime")
		{
			setDeadTime(parseWithErrorMsg<float>(tok, "Bad dead time value"));
		}
		else if (token == "bunching")
		{
			setBunching(parseWithErrorMsg<float>(tok, "Bad bunching value"));
		}
		else if (token == "color")
		{
			setColour(parseVector4(tok));
		}
		else if (token == "fadeColor")
		{
			setFadeColour(parseVector4(tok));
		}
		else if (token == "fadeIn")
		{
			setFadeInFraction(parseWithErrorMsg<float>(tok, "Bad fade in fraction value"));
		}
		else if (token == "fadeOut")
		{
			setFadeOutFraction(parseWithErrorMsg<float>(tok, "Bad fade out fraction value"));
		}
		else if (token == "fadeIndex")
		{
			setFadeIndexFraction(parseWithErrorMsg<float>(tok, "Bad fade index fraction value"));
		}
		else if (token == "animationFrames")
		{
			setAnimationFrames(parseWithErrorMsg<int>(tok, "Bad anim frames value"));
		}
		else if (token == "animationrate")
		{
			setAnimationRate(parseWithErrorMsg<float>(tok, "Bad anim rate value"));
		}
		else if (token == "angle")
		{
			setInitialAngle(parseWithErrorMsg<float>(tok, "Bad initial angle value"));
		}
		else if (token == "rotation")
		{
			_rotationSpeed.parseFromTokens(tok);
		}
		else if (token == "boundsExpansion")
		{
			setBoundsExpansion(parseWithErrorMsg<float>(tok, "Bad bounds expansion value"));
		}
		else if (token == "randomDistribution")
		{
			setRandomDistribution(tok.nextToken() == "1");
		}
		else if (token == "entityColor")
		{
			setUseEntityColour(tok.nextToken() == "1");
		}
		else if (token == "gravity")
		{
			// Get the next token. If it is "world", then parse another token for the actual strength
			token = tok.nextToken();

			if (token == "world")
			{
				setWorldGravityFlag(true);

				// Skip the "world" keyword and get the float
				token = tok.nextToken();
			}
			else
			{
				setWorldGravityFlag(false);
			}

			// At this point, the token contains the gravity factor, parse and assign it
			try
			{
				setGravity(boost::lexical_cast<float>(token));
			}
			catch (boost::bad_lexical_cast&)
			{
				globalErrorStream() << "[particles] Bad gravity value, token is '" << 
					token << "'" << std::endl;
			}
		}
		else if (token == "offset")
		{
			setOffset(parseVector3(tok));
		}
		else if (token == "speed")
		{
			_speed.parseFromTokens(tok);
		}
		else if (token == "orientation")
		{
			std::string orientationType = tok.nextToken();

			if (orientationType == "view")
			{
				setOrientationType(ORIENTATION_VIEW);
			}
			else if (orientationType == "aimed")
			{
				setOrientationType(ORIENTATION_AIMED);

				// Read orientation parameters
				setOrientationParm(0, parseWithErrorMsg<float>(tok, "Bad aimed param1 value"));
				setOrientationParm(1, parseWithErrorMsg<float>(tok, "Bad aimed param2 value"));
			}
			else if (orientationType == "x")
			{
				setOrientationType(ORIENTATION_X);
			}
			else if (orientationType == "y")
			{
				setOrientationType(ORIENTATION_Y);
			}
			else if (orientationType == "z")
			{
				setOrientationType(ORIENTATION_Z);
			}
			else
			{
				globalErrorStream() << "[particles] Unknown orientation type: " << 
					orientationType << std::endl;
			}	
		}
		else if (token == "distribution")
		{
			std::string distrType = tok.nextToken();

			if (distrType == "rect")
			{
				setDistributionType(DISTRIBUTION_RECT);

				// Read orientation parameters (sizex, sizey, sizez)
				setDistributionParm(0, parseWithErrorMsg<float>(tok, "Bad distr param1 value"));
				setDistributionParm(1, parseWithErrorMsg<float>(tok, "Bad distr param2 value"));
				setDistributionParm(2, parseWithErrorMsg<float>(tok, "Bad distr param3 value"));
			}
			else if (distrType == "cylinder")
			{
				setDistributionType(DISTRIBUTION_CYLINDER);

				// Read orientation parameters (sizex, sizey, sizez ringFraction?)
				setDistributionParm(0, parseWithErrorMsg<float>(tok, "Bad distr param1 value"));
				setDistributionParm(1, parseWithErrorMsg<float>(tok, "Bad distr param2 value"));
				setDistributionParm(2, parseWithErrorMsg<float>(tok, "Bad distr param3 value"));
				setDistributionParm(3, parseWithErrorMsg<float>(tok, "Bad distr param4 value"));
			}
			else if (distrType == "sphere")
			{
				setDistributionType(DISTRIBUTION_SPHERE);

				// Read orientation parameters (sizex, sizey, sizez)
				setDistributionParm(0, parseWithErrorMsg<float>(tok, "Bad distr param1 value"));
				setDistributionParm(1, parseWithErrorMsg<float>(tok, "Bad distr param2 value"));
				setDistributionParm(2, parseWithErrorMsg<float>(tok, "Bad distr param3 value"));
			}
			else
			{
				globalErrorStream() << "[particles] Unknown distribution type: " << 
					distrType << std::endl;
			}
		}
		else if (token == "direction")
		{
			std::string dirType = tok.nextToken();

			if (dirType == "cone")
			{
				setDirectionType(DIRECTION_CONE);

				// Read solid cone angle
				setDirectionParm(0, parseWithErrorMsg<float>(tok, "Bad cone angle value"));
			}
			else if (dirType == "outward")
			{
				setDirectionType(DIRECTION_OUTWARD);

				// Read upward bias
				setDirectionParm(0, parseWithErrorMsg<float>(tok, "Bad upward bias value"));
			}
			else
			{
				globalErrorStream() << "[particles] Unknown direction type: " << 
					dirType << std::endl;
			}
		}
		else if (token == "customPath")
		{
			std::string pathType = tok.nextToken();

			if (pathType == "helix")
			{
				setCustomPathType(PATH_HELIX);

				// Read helix parameters ( sizeX sizeY sizeZ radialSpeed axialSpeed )
				setCustomPathParm(0, parseWithErrorMsg<float>(tok, "Bad helix param1 value"));
				setCustomPathParm(1, parseWithErrorMsg<float>(tok, "Bad helix param2 value"));
				setCustomPathParm(2, parseWithErrorMsg<float>(tok, "Bad helix param3 value"));
				setCustomPathParm(3, parseWithErrorMsg<float>(tok, "Bad helix param4 value"));
				setCustomPathParm(4, parseWithErrorMsg<float>(tok, "Bad helix param5 value"));
			}
			else if (pathType == "flies")
			{
				setCustomPathType(PATH_FLIES);

				// Read flies parameters (radial_speed, axial_speed, size)
				setCustomPathParm(0, parseWithErrorMsg<float>(tok, "Bad flies param1 value"));
				setCustomPathParm(1, parseWithErrorMsg<float>(tok, "Bad flies param2 value"));
				setCustomPathParm(2, parseWithErrorMsg<float>(tok, "Bad flies param3 value"));
			}
			else if (pathType == "orbit")
			{
				setCustomPathType(PATH_ORBIT);

				// Read flies parameters (radius, speed)
				setCustomPathParm(0, parseWithErrorMsg<float>(tok, "Bad orbit param1 value"));
				setCustomPathParm(1, parseWithErrorMsg<float>(tok, "Bad orbit param2 value"));
			}
			else if (pathType == "drip")
			{
				setCustomPathType(PATH_DRIP);

				// Read flies parameters (something something) (sic!, as seen in the particle editor)
				setCustomPathParm(0, parseWithErrorMsg<float>(tok, "Bad drip param1 value"));
				setCustomPathParm(1, parseWithErrorMsg<float>(tok, "Bad drip param2 value"));
			}
			else
			{
				globalErrorStream() << "[particles] Unknown custom path type type: " << 
					pathType << std::endl;
			}
		}

		token = tok.nextToken();
	}
}

Vector3 ParticleStage::parseVector3(parser::DefTokeniser& tok)
{
	// Read 3 values and assemble to a Vector3
	Vector3 vec;

	try
	{
		vec.x() = boost::lexical_cast<float>(tok.nextToken());
		vec.y() = boost::lexical_cast<float>(tok.nextToken());
		vec.z() = boost::lexical_cast<float>(tok.nextToken());
	}
	catch (boost::bad_lexical_cast&)
	{
		vec = Vector3(0,0,0);

		globalErrorStream() << "[particles] Bad vector3 value." << std::endl;
	}

	return vec;
}

Vector4 ParticleStage::parseVector4(parser::DefTokeniser& tok)
{
	// Read 4 values and assemble to a Vector4
	Vector4 vec;

	try
	{
		vec.x() = boost::lexical_cast<float>(tok.nextToken());
		vec.y() = boost::lexical_cast<float>(tok.nextToken());
		vec.z() = boost::lexical_cast<float>(tok.nextToken());
		vec.w() = boost::lexical_cast<float>(tok.nextToken());
	}
	catch (boost::bad_lexical_cast&)
	{
		vec = Vector4(1,1,1,1);

		globalErrorStream() << "[particles] Bad vector4 value." << std::endl;
	}

	return vec;
}

} // namespace
