#include "StageDef.h"

#include "itextstream.h"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include "ParticleDef.h"

#include "shaderlib.h"

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
			rError() << "[particles] " << errorMsg << ", token is '" <<
				str << "'" << std::endl;
			return 0;
		}
	}

	inline std::ostream& writeColour(std::ostream& stream, const Vector4& colour)
	{
		stream << colour.x() << " " << colour.y() << " " << colour.z() << " " << colour.w();
		return stream;
	}

	inline std::ostream& writeVector3(std::ostream& stream, const Vector3& vec)
	{
		stream << vec.x() << " " << vec.y() << " " << vec.z();
		return stream;
	}
}

StageDef::StageDef() :
	_rotationSpeed(new ParticleParameter(*this)),
	_speed(new ParticleParameter(*this)),
	_size(new ParticleParameter(*this)),
	_aspect(new ParticleParameter(*this)),
	_visible(true)
{
	reset();
}

StageDef::StageDef(parser::DefTokeniser& tok) :
	_rotationSpeed(new ParticleParameter(*this)),
	_speed(new ParticleParameter(*this)),
	_size(new ParticleParameter(*this)),
	_aspect(new ParticleParameter(*this)),
	_visible(true)
{
	// Parse from the tokens, but don't allow any parse exceptions
	// from leaving this constructor.
	try
	{
		parseFromTokens(tok);
	}
	catch (parser::ParseException& p)
	{
		rError() << "[particles]: Could not parse particle stage: " <<
			p.what() << std::endl;
	}
	catch (boost::bad_lexical_cast& e)
	{
		rError() << "[particles]: Invalid cast when parsing particle stage: " <<
			e.what() << std::endl;
	}
}

void StageDef::reset()
{
	_count = 100;
	_material.clear();

	_duration = 1.5f;
	_cycles = 0;
	_bunching = 1.0f;

	_timeOffset = 0;
	_deadTime = 0;

	recalculateCycleMsec();

	_colour = Vector4(1,1,1,1);
	_fadeColour = Vector4(0,0,0,0);

	_fadeInFraction = 0.1f;		// 10% fade in by default
	_fadeOutFraction = 0.25f;	// 25% fade out by default
	_fadeIndexFraction = 0.0f;

	_animationFrames = 0;
	_animationRate = 0;

	_initialAngle = 0;

	_boundsExpansion = 0;

	_randomDistribution = true;
	_entityColor = false;

	_gravity = 1.0f;
	_applyWorldGravity = false;

	_orientationType = ORIENTATION_VIEW;
	_orientationParms[0] = _orientationParms[1] = _orientationParms[2] = _orientationParms[3] = 0;

	_distributionType = DISTRIBUTION_RECT;
	_distributionParms[0] = _distributionParms[1] = _distributionParms[2] = 8.0f; // 8x8x8 cube
	_distributionParms[3] = 0;

	_directionType = DIRECTION_CONE;
	_directionParms[0] = 90.0f;
	_directionParms[1] = _directionParms[2] = _directionParms[3] = 0;

	_customPathType = PATH_STANDARD;
	_customPathParms[0] = _customPathParms[1] = _customPathParms[2] = _customPathParms[3] = 0;
	_customPathParms[4] = _customPathParms[5] = _customPathParms[6] = _customPathParms[7] = 0;

	_speed.reset(new ParticleParameter(*this, 150.0f));
	_rotationSpeed.reset(new ParticleParameter(*this));
	_size.reset(new ParticleParameter(*this, 4.0f));
	_aspect.reset(new ParticleParameter(*this, 1.0f));
}

void StageDef::copyFrom(const IStageDef& other)
{
	setMaterialName(other.getMaterialName());
	setCount(other.getCount());
	setDuration(other.getDuration());
	setCycles(other.getCycles());
	setBunching(other.getBunching());
	setTimeOffset(other.getTimeOffset());
	setDeadTime(other.getDeadTime());
	setColour(other.getColour());
	setFadeColour(other.getFadeColour());
	setFadeInFraction(other.getFadeInFraction());
	setFadeOutFraction(other.getFadeOutFraction());
	setFadeIndexFraction(other.getFadeIndexFraction());
	setAnimationFrames(other.getAnimationFrames());
	setAnimationRate(other.getAnimationRate());
	setInitialAngle(other.getInitialAngle());
	setBoundsExpansion(other.getBoundsExpansion());
	setRandomDistribution(other.getRandomDistribution());
	setUseEntityColour(other.getUseEntityColour());
	setGravity(other.getGravity());
	setWorldGravityFlag(other.getWorldGravityFlag());
	setOffset(other.getOffset());
	setOrientationType(other.getOrientationType());

	for (int i = 0; i < 3; ++i)
	{
		setOrientationParm(i, other.getOrientationParm(i));
	}

	setDistributionType(other.getDistributionType());

	for (int i = 0; i < 3; ++i)
	{
		setDistributionParm(i, other.getDistributionParm(i));
	}

	setDirectionType(other.getDirectionType());

	for (int i = 0; i < 3; ++i)
	{
		setDirectionParm(i, other.getDirectionParm(i));
	}

	setCustomPathType(other.getCustomPathType());

	for (int i = 0; i < 7; ++i)
	{
		setCustomPathParm(i, other.getCustomPathParm(i));
	}

	*_size = other.getSize();
	*_aspect = other.getAspect();
	*_speed = other.getSpeed();
	*_rotationSpeed = other.getRotationSpeed();
}

void StageDef::parseFromTokens(parser::DefTokeniser& tok)
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
			_rotationSpeed->parseFromTokens(tok);
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
				rError() << "[particles] Bad gravity value, token is '" <<
					token << "'" << std::endl;
			}
		}
		else if (token == "offset")
		{
			setOffset(parseVector3(tok));
		}
		else if (token == "speed")
		{
			_speed->parseFromTokens(tok);
		}
		else if (token == "size")
		{
			_size->parseFromTokens(tok);
		}
		else if (token == "aspect")
		{
			_aspect->parseFromTokens(tok);
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
				rError() << "[particles] Unknown orientation type: " <<
					orientationType << std::endl;
			}
		}
		else if (token == "distribution")
		{
			std::string distrType = tok.nextToken();

			// We have old vanilla Doom 3 particles with upper case distribution types
			boost::algorithm::to_lower(distrType);

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

				// Try to parse that next value, some particle defs don't define a fourth component
				std::string nextToken = tok.peek();

				try
				{
					float parm = boost::lexical_cast<float>(nextToken);

					// successfully converted the next token to a number
					setDistributionParm(3, parm);

					// Skip that next token
					tok.skipTokens(1);
				}
				catch (boost::bad_lexical_cast&)
				{
					setDistributionParm(3, 0.0f);
				}
			}
			else if (distrType == "sphere")
			{
				setDistributionType(DISTRIBUTION_SPHERE);

				// Read orientation parameters (sizex, sizey, sizez)
				setDistributionParm(0, parseWithErrorMsg<float>(tok, "Bad distr param1 value"));
				setDistributionParm(1, parseWithErrorMsg<float>(tok, "Bad distr param2 value"));
				setDistributionParm(2, parseWithErrorMsg<float>(tok, "Bad distr param3 value"));

				// Try to parse that next value, the D3 particle editor won't save the 4th parameter
				std::string nextToken = tok.peek();

				try
				{
					float parm = boost::lexical_cast<float>(nextToken);

					// successfully converted the next token to a number
					setDistributionParm(3, parm);

					// Skip that next token
					tok.skipTokens(1);
				}
				catch (boost::bad_lexical_cast&)
				{
					setDistributionParm(3, 0.0f);
				}
			}
			else
			{
				rError() << "[particles] Unknown distribution type: " <<
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
				rError() << "[particles] Unknown direction type: " <<
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

				// Read orbit parameters (radius, speed)
				// These are actually unsupported by the engine ("bad path type")
				setCustomPathParm(0, parseWithErrorMsg<float>(tok, "Bad orbit param1 value"));
				setCustomPathParm(1, parseWithErrorMsg<float>(tok, "Bad orbit param2 value"));
			}
			else if (pathType == "drip")
			{
				setCustomPathType(PATH_DRIP);

				// Read drip parameters (something something) (sic!, as seen in the particle editor)
				// These are actually unsupported by the engine ("bad path type")
				setCustomPathParm(0, parseWithErrorMsg<float>(tok, "Bad drip param1 value"));
				setCustomPathParm(1, parseWithErrorMsg<float>(tok, "Bad drip param2 value"));
			}
			else
			{
				rError() << "[particles] Unknown custom path type type: " <<
					pathType << std::endl;
			}
		}

		token = tok.nextToken();
	}
}

Vector3 StageDef::parseVector3(parser::DefTokeniser& tok)
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

		rError() << "[particles] Bad vector3 value." << std::endl;
	}

	return vec;
}

Vector4 StageDef::parseVector4(parser::DefTokeniser& tok)
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

		rError() << "[particles] Bad vector4 value." << std::endl;
	}

	return vec;
}

std::ostream& operator<<(std::ostream& stream, const StageDef& stage)
{
	std::size_t prevPrecision = stream.precision();

	// Three post-comma digits precision
	stream.precision(3);

	// Opening brace
	stream << "\t{" << std::endl;

	stream << "\t\t" << "count " << "\t\t\t\t" << stage.getCount() << std::endl;

	const std::string& material = stage.getMaterialName();
	stream << "\t\t" << "material " << "\t\t\t" << (material.empty() ? GlobalTexturePrefix_get() : material) << std::endl;

	if (stage.getAnimationFrames() != 0)
	{
		stream << "\t\t" << "animationFrames " << "\t" << stage.getAnimationFrames() << std::endl;
	}

	if (stage.getAnimationRate() != 0)
	{
		stream << "\t\t" << "animationrate " << "\t\t" << stage.getAnimationRate() << std::endl;
	}

	stream << "\t\t" << "time " << "\t\t\t\t" << stage.getDuration() << std::endl;
	stream << "\t\t" << "cycles " << "\t\t\t\t" << stage.getCycles() << std::endl;

	if (stage.getDeadTime() != 0)
	{
		stream << "\t\t" << "deadTime " << "\t\t\t" << stage.getDeadTime() << std::endl;
	}

	stream << "\t\t" << "timeOffset " << "\t\t\t" << stage.getTimeOffset() << std::endl;
	stream << "\t\t" << "bunching " << "\t\t\t" << stage.getBunching() << std::endl;

	// Distribution
	stream << "\t\t" << "distribution " << "\t\t";

	switch (stage.getDistributionType())
	{
	case StageDef::DISTRIBUTION_RECT:
		stream << "rect " << stage.getDistributionParm(0) << " "
						  << stage.getDistributionParm(1) << " "
						  << stage.getDistributionParm(2) << std::endl;
		break;
	case StageDef::DISTRIBUTION_CYLINDER:
		stream << "cylinder " << stage.getDistributionParm(0) << " "
							  << stage.getDistributionParm(1) << " "
							  << stage.getDistributionParm(2) << " "
							  << stage.getDistributionParm(3) << std::endl;
		break;
	case StageDef::DISTRIBUTION_SPHERE:
		stream << "sphere " << stage.getDistributionParm(0) << " "
							<< stage.getDistributionParm(1) << " "
							<< stage.getDistributionParm(2) << std::endl;
		break;
	};

	// Direction
	stream << "\t\t" << "direction " << "\t\t\t";

	switch (stage.getDirectionType())
	{
	case StageDef::DIRECTION_CONE:
		stream << "cone";
		break;
	case StageDef::DIRECTION_OUTWARD:
		stream << "outward";
		break;
	};

	stream << " " << stage.getDirectionParm(0) << std::endl;

	// Orientation
	stream << "\t\t" << "orientation " << "\t\t";

	switch (stage.getOrientationType())
	{
	case StageDef::ORIENTATION_VIEW:
		stream << "view";
		break;
	case StageDef::ORIENTATION_AIMED:
		stream << "aimed " << stage.getOrientationParm(0) << " " << stage.getOrientationParm(1); // trails + time
		break;
	case StageDef::ORIENTATION_X:
		stream << "x";
		break;
	case StageDef::ORIENTATION_Y:
		stream << "y";
		break;
	case StageDef::ORIENTATION_Z:
		stream << "z";
		break;
	};

	stream << std::endl;

	// Custom path, only flies and helix get written
	switch (stage.getCustomPathType())
	{
	case StageDef::PATH_FLIES: // customPath flies 10.000 10.000 10.000
		stream << "\t\t" << "customPath " << "\t\t\t";
		stream << "flies " << stage.getCustomPathParm(0) << " "
						   << stage.getCustomPathParm(1) << " "
						   << stage.getCustomPathParm(2) << std::endl;
		break;
	case StageDef::PATH_HELIX:
		stream << "\t\t" << "customPath " << "\t\t\t";
		stream << "helix " << stage.getCustomPathParm(0) << " "
						   << stage.getCustomPathParm(1) << " "
						   << stage.getCustomPathParm(2) << " "
						   << stage.getCustomPathParm(3) << " "
						   << stage.getCustomPathParm(4) << std::endl;
		break;
	default:
		break;
	};

	stream << "\t\t" << "speed " << "\t\t\t\t" << stage.getSpeed() << std::endl;
	stream << "\t\t" << "size " << "\t\t\t\t" << stage.getSize() << std::endl;
	stream << "\t\t" << "aspect " << "\t\t\t\t" << stage.getAspect() << std::endl;

	if (stage.getInitialAngle() != 0)
	{
		stream << "\t\t" << "angle " << "\t\t\t\t" << stage.getInitialAngle() << std::endl;
	}

	stream << "\t\t" << "rotation " << "\t\t\t" << stage.getRotationSpeed() << std::endl;
	stream << "\t\t" << "randomDistribution " << "\t" << (stage.getRandomDistribution() ? "1" : "0") << std::endl;
	stream << "\t\t" << "boundsExpansion " << "\t" << stage.getBoundsExpansion() << std::endl;
	stream << "\t\t" << "fadeIn " << "\t\t\t\t" << stage.getFadeInFraction() << std::endl;
	stream << "\t\t" << "fadeOut " << "\t\t\t" << stage.getFadeOutFraction() << std::endl;
	stream << "\t\t" << "fadeIndex " << "\t\t\t" << stage.getFadeIndexFraction() << std::endl;

	stream << "\t\t" << "color " << "\t\t\t\t";
	writeColour(stream, stage.getColour());
	stream << std::endl;

	stream << "\t\t" << "fadeColor " << "\t\t\t";
	writeColour(stream, stage.getFadeColour());
	stream << std::endl;

	stream << "\t\t" << "offset " << "\t\t\t\t";
	writeVector3(stream, stage.getOffset());
	stream << std::endl;

	stream << "\t\t" << "gravity " << "\t\t\t" << (stage.getWorldGravityFlag() ? "world " : "") << stage.getGravity() << std::endl;

	if (stage.getUseEntityColour())
	{
		stream << "\t\t" << "entityColor " << "\t\t" << "1" << std::endl;
	}

	// Closing brace
	stream << "\t}" << std::endl;

	stream.precision(prevPrecision);

	return stream;
}

void StageDef::setMaterialName(const std::string& material)
{
    _material = material;
    _changedSignal.emit();
}

bool StageDef::operator==(const IStageDef& other) const
{
    if (getMaterialName() != other.getMaterialName()) return false;

    if (getCount() != other.getCount()) return false;
    if (getDuration() != other.getDuration()) return false;
    if (getCycles() != other.getCycles()) return false;
    if (getBunching() != other.getBunching()) return false;
    if (getTimeOffset() != other.getTimeOffset()) return false;
    if (getDeadTime() != other.getDeadTime()) return false;
    if (getColour() != other.getColour()) return false;
    if (getFadeColour() != other.getFadeColour()) return false;
    if (getFadeInFraction() != other.getFadeInFraction()) return false;
    if (getFadeOutFraction() != other.getFadeOutFraction()) return false;
    if (getFadeIndexFraction() != other.getFadeIndexFraction()) return false;
    if (getAnimationFrames() != other.getAnimationFrames()) return false;
    if (getAnimationRate() != other.getAnimationRate()) return false;
    if (getInitialAngle() != other.getInitialAngle()) return false;
    if (getBoundsExpansion() != other.getBoundsExpansion()) return false;
    if (getRandomDistribution() != other.getRandomDistribution()) return false;
    if (getUseEntityColour() != other.getUseEntityColour()) return false;
    if (getGravity() != other.getGravity()) return false;
    if (getWorldGravityFlag() != other.getWorldGravityFlag()) return false;
    if (getOffset() != other.getOffset()) return false;
    if (getOrientationType() != other.getOrientationType()) return false;

    for (int i = 0; i < 3; ++i)
    {
        if (getOrientationParm(i) != other.getOrientationParm(i)) return false;
    }

    if (getDistributionType() != other.getDistributionType()) return false;

    for (int i = 0; i < 3; ++i)
    {
        if (getDistributionParm(i) != other.getDistributionParm(i)) return false;
    }

    if (getDirectionType() != other.getDirectionType()) return false;

    for (int i = 0; i < 3; ++i)
    {
        if (getDirectionParm(i) != other.getDirectionParm(i)) return false;
    }

    if (getCustomPathType() != other.getCustomPathType()) return false;

    for (int i = 0; i < 7; ++i)
    {
        if (getCustomPathParm(i) != other.getCustomPathParm(i)) return false;
    }

    if (getSize() != other.getSize()) return false;
    if (getAspect() != other.getAspect()) return false;
    if (getSpeed() != other.getSpeed()) return false;
    if (getRotationSpeed() != other.getRotationSpeed()) return false;

    // All checks passed => equal
    return true;
}
} // namespace
