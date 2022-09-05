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
	}
}