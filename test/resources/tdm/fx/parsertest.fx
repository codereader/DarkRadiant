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
		delay 0
		sound "security_camera_spark"
		duration 2.5

	}
	{
		delay 0
		duration 2.5
		restart 0
		model "sparks.prt"
	}		   
}
