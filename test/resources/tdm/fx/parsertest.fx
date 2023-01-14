fx fx/tdm_flame
{
	bindto "Head"
        {
		delay 0
		duration 10
		restart 0
		light "lights/roundfire", 0.2, 0.6, 0.3, 300
		offset	0, 0, 5
		//fadeIn  2
                fadeOut 3
	}
}

fx fx/sparks
{
	{
		delay 0
		duration 0.5
		restart 0
		light "lights/sparks_sound", 0.7, 0.7, 0.7, 64
		offset	0, 0, 0
        trackOrigin 0
	}

	{
		delay 2
		sound "security_camera_spark"
		duration 2.5

	}
	{
		delay 1.5
		duration 2.5
		restart 0
		model "sparks.prt"
	}		   
}

fx fx/parserTest1
{
    {
        delay 0
        ignoreMaster
        duration 0.5
        restart 0
        light "lights/sparks_sound", 0.7, 0.7, 0.7, 64
        offset	0, 0, 0
        noshadows
    }
}

fx fx/parserTest/shake
{
	{
        // place the commas in a slightly random fashion
        shake 1.3, 2.7, 0.7 , 1 , 0.33
	}
    {
        // place the commas in a slightly random fashion
        shake 0.0, 2, .7 , 0 , 0.33
    }
}

fx fx/parserTest/name
{
	{
        name "Testaction"
        delay 1.5
        duration 2.5
        restart 0
        model "sparks.prt"
	}
}

fx fx/parserTest/sibling
{
	{
        name "Testaction"
        delay 1.5
        duration 2.5
        restart 0
        fire "SisterAction"
        model "sparks.prt"
	}
	{
        name "SisterAction"
        delay 2.5
        random 0.9,6.888
        model "sparks.prt"
        rotate 56.7
        duration 0.8
        restart 1
        trackOrigin 1
	}
}

fx fx/parserTest/fadeIn
{
	{
        name "FadeIn"
        delay 1.5
        duration 2.5
        fadeIn 1.2
	}
	{
        name "FadeOut"
        fadeOut 0.8
        size 1.5
        offset 1.6  , 0.7,-0.8
	}
}

fx fx/parserTest/axisAndAngle
{
	{
        delay 1.5
        axis 0.8,0.6, 0.5
	}
	{
        delay 0.5
        angle 0.8,-0.6, 0.2
	}
}

fx fx/parserTest/useLight
{
	{
        delay 1.5
        name "LightOwner"
        light "lights/biground", 0.5, 1, 0.7, 550.3
	}
	{
        useLight "LightOwner"
        fadeOut 0.5
	}
}

fx fx/parserTest/useModel
{
	{
        delay 1.5
        model "tree.ase"
        name "ModelOwner"
	}
	{
        useModel "ModelOwner"
        fadeOut 0.5
	}
    {
        particle "drips.prt"
        fadeOut 0.5
        particleTrackVelocity
    }
}

fx fx/parserTest/attach
{
	{
        delay 1.5
        attachLight "light_1"
	}
    {
        delay 2.5
        attachEntity "func_static_1"
    }
}

fx fx/parserTest/projectile
{
	{
        delay 1.5
        launch "atdm:projectile_broadhead"
	}
}

fx fx/parserTest/decal
{
	{
        delay 1.5
        decal "textures/decals/blood"
	}
}

fx fx/parserTest/sound
{
	{
        delay 1.5
        sound "footsteps/stone"
	}
}

fx fx/parserTest/shockwave
{
	{
        delay 1.5
        shockwave "atdm:some_shockwave_def"
	}
}