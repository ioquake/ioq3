// q3map_sun <red> <green> <blue> <intensity> <degrees> <elevation>
// color will be normalized, so it doesn't matter what range you use
// intensity falls off with angle but not distance 100 is a fairly bright sun
// degree of 0 = from the east, 90 = north, etc.  altitude of 0 = sunrise/set, 90 = noon

// skyparms work like this: 

textures/skies/xtoxicsky_q3ctf3
{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	qer_editorimage textures/skies/toxicbluesky.tga
	q3map_surfacelight 500
	q3map_sun	1 1 0.5 150	30 60
	skyparms - 512 -
	{
		map textures/skies/bluedimclouds.tga
		tcMod scale 3 2
		tcMod scroll 0.15 0.15
		depthWrite
	}
	{
		map textures/skies/topclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scale 3 3
		tcMod scroll 0.05 0.05
	}
}


//*******************************************************
//*              Protoskyboxattempt  
//*******************************************************
textures/skies/skybox
{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
        q3map_lightimage textures/skies/stars.tga
	q3map_sun	1 1 1 100 -58 58
	q3map_surfacelight 400

        skyparms env/space1 - -
       {
		map textures/skies/killsky_2.tga
		blendfunc GL_ONE GL_ONE
		tcMod scroll 0.05 0.06
		tcMod scale 3 2
	}
}


textures/skies/xtoxicskytim_q3dm5
//used in tim_dm2

{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

//	q3map_sun	.5 .6 .8 100 190 75
//	q3map_surfacelight 120

	q3map_surfacelight 400
	q3map_sun	.5 .6 .8 150 30 60


	qer_editorimage textures/skies/toxicbluesky.tga

	skyparms - 512 -
//	skyparms - 512 full

	//cloudparms 512 full
	{
		map textures/skies/bluedimclouds.tga
		tcMod scale 3 2
		tcMod scroll 0.15 0.15
		depthWrite
	}
	{
		map textures/skies/topclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scale 3 3
		tcMod scroll 0.05 0.05
	}
}

textures/skies/blacksky
{
	qer_editorimage textures/skies/blacksky.tga
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	//skyparms env/q3sky_red1 - -
	//skyparms env/kmnight - -
	//skyparms env/black - -
	q3map_surfacelight 25
	q3map_lightsubdivide 512
	q3map_sun	1 1 1 32	90 90
	{
		map	gfx/colors/black.tga
	}
}

textures/skies/xblacksky
{
	qer_editorimage textures/skies/blacksky.tga
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	//skyparms env/q3sky_red1 - -
	//skyparms env/kmnight - -
	//skyparms env/black - -
	q3map_surfacelight 300
	q3map_lightsubdivide 512
	q3map_sun	1 1 1 32	45 90
	{
		map	gfx/colors/black.tga
	}
}


textures/skies/xblacksky_dm7
{
	qer_editorimage textures/skies/blacksky.tga
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	//skyparms env/q3sky_red1 - -
	//skyparms env/kmnight - -
	//skyparms env/black - -
//	q3map_surfacelight 300
//	q3map_lightsubdivide 512
//	q3map_sun	1 1 1 32	45 90
	{
		map	gfx/colors/black.tga
	}
}

textures/skies/stars_arena7
{
	qer_editorimage textures/skies/stars.tga

	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap

	q3map_surfacelight 10
	q3map_sun .6 .6 .6 30 35 78

	skyparms env/black - -
}

textures/skies/killsky
{
	qer_editorimage textures/skies/stars.tga
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm sky

	q3map_sun 3 2 2 70 315 65
	q3map_surfacelight 75
	skyparms - 512 -

	//cloudparms 512 full
	//lightning

	{
		map textures/skies/killsky_1.tga
		tcMod scroll 0.05 .1
		tcMod scale 2 2
	}
	{
		map textures/skies/killsky_2.tga
		blendfunc GL_ONE GL_ONE
		tcMod scroll 0.05 0.06
		tcMod scale 3 2
	}
}

textures/skies/tim_km2
{
	qer_editorimage textures/skies/stars_red.tga
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm sky

	q3map_sun .5 .37 .19 90 315 40

	//q3map_sun 1 .37 .19 70 30 70
	q3map_surfacelight 80
	skyparms - 512 -

	//cloudparms 512 full
	//lightning

	{
		map textures/skies/killsky_1.tga
		tcMod scroll 0.05 .1
		tcMod scale 2 2
		depthWrite
	}
	{
		map textures/skies/killsky_2.tga
		blendfunc GL_ONE GL_ONE
		tcMod scroll 0.05 0.06
		tcMod scale 3 2
	}
}

textures/skies/tim_hell
{
	qer_editorimage textures/skies/stars_red.tga
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm sky

	q3map_sun 1 .77 .77 80 315 70
	//q3map_sun .5 .37 .19 80 315 70

	//q3map_sun 1 .37 .19 85 30 70
	q3map_surfacelight 130
	skyparms - 384 -

	//cloudparms 512 full
	//lightning

	{
		map textures/skies/killsky_1.tga
		tcMod scroll 0.05 .1
		tcMod scale 2 2
		depthWrite
	}
	{
		map textures/skies/killsky_2.tga
		blendfunc GL_ONE GL_ONE
		tcMod scroll 0.05 0.06
		tcMod scale 3 2
	}
}

textures/skies/tim_dm3_red
{
	qer_editorimage textures/skies/stars_red.tga
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm sky

	q3map_sun .5 .37 .19 70 30 70

	//q3map_sun 1 .37 .19 70 30 70
	q3map_surfacelight 80
	skyparms - 512 -

	//cloudparms 512 full
	//lightning

	{
		map textures/skies/killsky_1.tga
		tcMod scroll 0.05 .1
		tcMod scale 2 2
		depthWrite
	}
	{
		map textures/skies/killsky_2.tga
		blendfunc GL_ONE GL_ONE
		tcMod scroll 0.05 0.06
		tcMod scale 3 2
	}
}

textures/skies/tim_dm14
{
	qer_editorimage textures/skies/stars.tga
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm sky

	q3map_sun 3 2 2 90 314 60
	q3map_surfacelight  80 
	skyparms - 512 -

	//cloudparms 512 full
	//lightning

	{
		map textures/skies/killsky_1.tga
		tcMod scroll 0.05 .1
		tcMod scale 2 2
		depthWrite
	}
	{
		map textures/skies/killsky_2.tga
		blendfunc GL_ONE GL_ONE
		tcMod scroll 0.05 0.06
		tcMod scale 3 2
	}
}

textures/skies/tim_dm14red
// test sky for q3dm15b
{
	qer_editorimage textures/skies/stars_red.tga
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm sky

	q3map_sun .5 .37 .19 70 314 60

	//q3map_sun 1 .37 .19 70 30 70
	q3map_surfacelight 80
	skyparms - 512 -

	{
		map textures/skies/killsky_1.tga
		tcMod scroll 0.05 .1
		tcMod scale 2 2
		depthWrite
	}
	{
		map textures/skies/killsky_2.tga
		blendfunc GL_ONE GL_ONE
		tcMod scroll 0.05 0.06
		tcMod scale 3 2
	}
}

textures/skies/hellsky
{
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm sky

	q3map_sun 4 3 3 65 290 75
	q3map_surfacelight 50
	skyparms - 512 -

	{
		map textures/skies/dimclouds.tga
		tcMod scroll 0.05 0
		tcMod scale 2 2
		depthWrite
	}
	//{
	//	map textures/skies/dimclouds.tga
	//	blendfunc GL_ONE GL_ONE
	//	tcMod scroll 0.05 0.06
		//tcMod scale 1 1
	//}
}

textures/skies/toxicsky
{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	q3map_surfacelight 80
	q3map_sun	1 1 0.5 100	30 60

//	cloudparms 512 full

	skyparms - 512 -

	{
		map textures/skies/inteldimclouds.tga
		tcMod scroll 0.1 0.1
		tcMod scale 3 2
		depthWrite
	}
	{
		map textures/skies/intelredclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scroll 0.05 0.05
		tcMod scale 3 3
	}
}

textures/skies/xtoxicsky_dm3
{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	qer_editorimage textures/skies/toxicsky.tga

	q3map_surfacelight 400
	q3map_sun	1 1 0.5 150	30 60
//	q3map_sun	1 1 0.5 150	200 85
	skyparms - 512 -

	{
		map textures/skies/inteldimclouds.tga
		tcMod scroll 0.1 0.1
		tcMod scale 3 2
		depthWrite
	}
	{
		map textures/skies/intelredclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scroll 0.05 0.05
		tcMod scale 3 3
	}
}



textures/skies/xtoxicsky_dm9
{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	qer_editorimage textures/skies/toxicsky.tga

	q3map_surfacelight 400
	q3map_sun	1 1 0.5 150	30 60
	skyparms - 512 -

	{
		map textures/skies/inteldimclouds.tga
		tcMod scroll 0.1 0.1
		tcMod scale 3 2
		depthWrite
	}
	{
		map textures/skies/intelredclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scroll 0.05 0.05
		tcMod scale 3 3
	}
}

textures/skies/xtoxicsky_ctf
{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	qer_editorimage textures/skies/toxicsky.tga

	q3map_surfacelight 500
	q3map_sun	1 1 0.5 150	30 60
	skyparms - 512 -

	{
		map textures/skies/inteldimclouds.tga
		tcMod scroll 0.1 0.1
		tcMod scale 3 2
		depthWrite
	}
	{
		map textures/skies/intelredclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scroll 0.05 0.05
		tcMod scale 3 3
	}
}

textures/skies/xtoxicsky_tourney
{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	qer_editorimage textures/skies/toxicsky.tga

	q3map_surfacelight 495
//	q3map_sun	1 1 0.5 150	55 60
	q3map_sun	1 1 0.5 150	75 60
	skyparms - 512 -

	{
		map textures/skies/inteldimclouds.tga
		tcMod scroll 0.1 0.1
		tcMod scale 3 2
		depthWrite
	}
	{
		map textures/skies/intelredclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scroll 0.05 0.05
		tcMod scale 3 3
	}
}

textures/skies/nightsky_xian_dm1
{
	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap

	q3map_surfacelight 300
	q3map_sun 1 1 1 100 -41 58

	qer_editorimage textures/skies/nightsky_nocloud.tga
//        skyparms env/space1 - -


	skyparms env/xnight2 - -
}

textures/skies/nightsky_xian_dm2
{
	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap

	q3map_surfacelight 200
	q3map_sun 1 1 1 100 -41 58

	qer_editorimage textures/skies/nightsky_nocloud.tga

	sky	env/xnight2
}

textures/skies/nightsky_xian_dm3
{
	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap

	q3map_surfacelight 500
	q3map_sun 1 1 1 100 -41 58

	qer_editorimage textures/skies/nightsky_nocloud.tga

	sky	env/xnight2
}

textures/skies/nightsky_xian_dm15
{
	qer_editorimage textures/skies/nightsky_nocloud.tga
	surfaceparm nolightmap
	surfaceparm sky
	surfaceparm noimpact

	q3map_surfacelight 900
	q3map_sun 1 1 1 100 -58 58
	skyparms env/xnight2 - -

//	q3map_sun 1 1 1 100 -41 58
//	q3map_sun 1 1 1 100 -49 58

}



textures/skies/xblacksky
{
	qer_editorimage textures/skies/blacksky.tga
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	q3map_surfacelight 2
	q3map_sun	0.2 0.2 0.3 80 -55 75

}

textures/skies/xtoxicsky_dm4
{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	q3map_surfacelight 80
	q3map_sun	1 1 0.95 50 40 90

	qer_editorimage textures/skies/toxicsky.tga

	//sky env/hell2
	cloudparms 512 full
	//lightning
	{
		map textures/skies/inteldimclouds.tga
		tcMod scroll 0.1 0.1
		tcMod scale 3 2
		depthWrite
	}
	{
		map textures/skies/intelredclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scroll 0.05 0.05
		tcMod scale 3 3
	}
}


textures/skies/xtoxicsky_dm14
{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	q3map_surfacelight 800
//	q3map_sun	1 1 0.5 100	75 45
//	q3map_sun	1 1 0.85 100	140 85
	q3map_sun	1 1 0.85 100	90 90


	qer_editorimage textures/skies/toxicsky.tga

	skyparms - 512 -
	{
		map textures/skies/inteldimclouds.tga
		tcMod scroll 0.1 0.1
		tcMod scale 3 2
		depthWrite
	}
	{
		map textures/skies/intelredclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scroll 0.05 0.05
		tcMod scale 3 3
	}
}


textures/skies/xtoxicsky_90_60
{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	q3map_surfacelight 80
//	q3map_sun	1 1 0.5 100	75 45
	q3map_sun	1 1 0.5 200	160 70
	qer_editorimage textures/skies/toxicsky.tga

	//sky env/hell2
	cloudparms 512 full
	//lightning
	{
		map textures/skies/inteldimclouds.tga
		tcMod scroll 0.1 0.1
		tcMod scale 3 2
		depthWrite
	}
	{
		map textures/skies/intelredclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scroll 0.05 0.05
		tcMod scale 3 3
	}
}

textures/skies/space_holder
{
	qer_editorimage textures/skies/space_holder.tga
	//surfaceparm sky
	surfaceparm noimpact
	//q3map_surfacelight 300
	surfaceparm nolightmap
	sky	env/space1
}
textures/skies/nightsky_nocloud_600
{
	qer_editorimage textures/skies/nightsky_nocloud.tga
	surfaceparm sky
	surfaceparm noimpact
	q3map_surfacelight 600
	surfaceparm nolightmap
	sky	env/xnight2
}
textures/skies/nightsky_nocloud_100
{
	qer_editorimage textures/skies/nightsky_nocloud.tga
	surfaceparm sky
	surfaceparm noimpact
	//q3map_surfacelight 100
	q3map_sun 1 1 1 100 -41 58
	q3map_surfacelight 300
	surfaceparm nolightmap
	sky	env/xnight2
}
textures/skies/nightsky_nocloud_200
{
	qer_editorimage textures/skies/nightsky_nocloud.tga
	surfaceparm sky
	surfaceparm noimpact
	q3map_surfacelight 200
	surfaceparm nolightmap
	sky	env/xnight2
}

textures/skies/nightsky_nocloud_1000
{
	qer_editorimage textures/skies/nightsky_nocloud.tga
	surfaceparm sky
	surfaceparm noimpact
	q3map_surfacelight 1000
	surfaceparm nolightmap
	sky	env/xnight2
}

textures/skies/nightsky_nocloud_500
{
	qer_editorimage textures/skies/nightsky_nocloud.tga
	surfaceparm sky
	surfaceparm noimpact
	q3map_surfacelight 500
	surfaceparm nolightmap
	sky	env/xnight2
}
textures/skies/nightsky_nocloud_400
{
	qer_editorimage textures/skies/nightsky_nocloud.tga
	surfaceparm sky
	surfaceparm noimpact
	q3map_surfacelight 400
	surfaceparm nolightmap
	sky	env/xnight2
}
textures/skies/nightsky_nocloud
{
	surfaceparm sky
	surfaceparm noimpact
	q3map_surfacelight 350
	surfaceparm nolightmap
	sky	env/xnight2
}
textures/skies/hellredclouds
{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	q3map_globaltexture
	q3map_lightsubdivide 512 

	q3map_sun 5 2 2 60 135 90
	q3map_surfacelight 80

	skyparms - 512 -

	{
		map textures/skies/redclouds.tga
		tcMod scroll 0.02 0
		tcMod scale 2 2
	}
	{
		map textures/skies/lightningsky8_kc.tga
		blendfunc GL_ONE GL_ONE
		tcMod scale 10 10
		tcMod scroll .2 .2
	}	
	{
		map textures/skies/redcloudsa.tga
		blendFunc GL_ONE GL_ONE
		tcMod scale 3 3
		tcMod scroll 0.02 0.01
	}


}
textures/skies/hellskybright
{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	q3map_sun 4 3 3 80 45 75
	q3map_surfacelight 60

	skyparms - 512 -
	{
		map textures/skies/dimclouds.tga
		//blendFunc GL_ONE GL_ONE
		tcMod scroll 0.05 0
		//tcMod scroll 0 0
		tcMod scale 2 2
		depthWrite
	}
	{
		map textures/skies/redclouds.tga
		//blendFunc GL_DST_COLOR GL_ZERO
		blendFunc GL_ONE GL_ONE
		tcMod scroll 0.05 0.06
		tcMod scale 1 1
	}
}
//Old sky, should be replaced with toxicsky
textures/skies/hellsky2
{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	q3map_sun .9 .9 1 70 65 78
	q3map_surfacelight 60
	skyparms - 512 -
	{
		map textures/skies/dimclouds.tga
		tcMod scroll 0.02 0
		tcMod scale 1 1
		depthWrite
	}
	{
		map textures/skies/dimclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scroll 0.02 0.01
		tcMod scale 3 3
	}

}
textures/skies/hellsky2bright
{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	q3map_sun	1 1 1 100 220 50
	q3map_surfacelight 120

	qer_editorimage textures/skies/toxicsky.tga

	skyparms - 512 -
	{
		map textures/skies/inteldimclouds.tga
		tcMod scale 3 2
		tcMod scroll 0.15 0.15
		depthWrite
	}
	{
		map textures/skies/intelredclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scale 3 3
		tcMod scroll 0.05 0.05
	}
}

textures/skies/hellsky
{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	q3map_sun 5 3 3 60 290 75
	q3map_surfacelight 90

	cloudparms 512 full
	lightning

	{
		map textures/skies/redclouds.tga
		tcMod scroll 0.05 0
		tcMod scale 2 2
		depthWrite
	}
	{
		map textures/skies/dimclouds.tga
		blendfunc GL_ONE GL_ONE
		tcMod scroll 0.05 0.06
		tcMod scale 1 1
	}
}
textures/hell/newhell
{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	q3map_surfacelight 200

	sky env/hell2
	cloudparms 512 full
	lightning

	{
		map env/redcloudsa.tga
		blendFunc GL_ONE GL_ONE
		tcMod scroll 0.05 0
		tcMod scale 2 2
	}
	{
		map env/redclouds.tga
		blendFunc GL_DST_COLOR GL_ZERO
		tcMod scroll 0.05 0.06
		tcMod scale 1 1
	}
}

//*******************************************************
//*              Jaquays/steed  environment test 1
//*******************************************************
//textures/skies/red_enviro1
//{
//	qer_editorimage textures/test/q3envtest.tga

//	surfaceparm noimpact
//	surfaceparm nolightmap
//	surfaceparm sky

//	q3map_sun	0.933333 0.541176 03.13725 60 160 11
//	q3map_surfacelight 10

//	sky env/q3envtest

	

	//{
		//map textures/skies/dimclouds.tga
		
		//tcMod turb 0 0.001 0.5 0.001
		//tcMod scale 3 3
		//tcMod scroll 0.01 0.01
		//blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
		//depthWrite
	//}
	//{
		//map textures/skies/pjbasesky.tga
		//blendfunc GL_ONE GL_ONE
		//tcMod scroll -0.01 -0.01
		//tcMod scale 5 5
	//}
//}

//*******************************************************
//*              Jaquays/steed  environment test 2
//*******************************************************
//textures/skies/blue_enviro2
//{
//	qer_editorimage textures/test/q3env2.tga

//	surfaceparm noimpact
//	surfaceparm nolightmap
//	surfaceparm sky

//	q3map_sun	1 1 1 50 160 15
//	q3map_surfacelight 20
//
//	cloudparms 1024 full
//	sky env/q3env2
	

	//{
		//map textures/test/spacerocks.tga
		
		//tcMod turb 0 0.001 0.5 0.0001
		//tcMod scale 0.5 0.5
		//tcMod scroll 0.001 0.01
		//blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
		//depthWrite
	//}
	//{
		//map textures/skies/pjbasesky.tga
		//blendfunc GL_ONE GL_ONE
		//tcMod scroll -0.01 -0.01
		//tcMod scale 5 5
	//}
//}

//*******************************************************
//*              Paul Jaquays' pj_dm10 sky
//*******************************************************
textures/skies/pj_dm10sky
{
	qer_editorimage textures/skies/pjbasesky.tga
	q3map_lightsubdivide 512 
	q3map_globaltexture
	surfaceparm noimpact
	surfaceparm nolightmap

	q3map_sun	.5 .6 .8 90 225 55
	q3map_surfacelight 150

	qer_editorimage textures/skies/toxicbluesky.tga

	skyparms - 512 -

	{
		map textures/skies/bluedimclouds.tga
		tcMod scale 3 2
		tcMod scroll 0.15 0.15
		depthWrite
	}
	{
		map textures/skies/topclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scale 3 3
		tcMod scroll 0.05 0.05
	}

}

//*******************************************************
//*              Kevin Cloud's kc_dm10 sky
//*******************************************************
textures/skies/kc_dm10sky
{
	qer_editorimage textures/skies/pjbasesky.tga
	q3map_lightsubdivide 512 
	q3map_globaltexture
	surfaceparm noimpact
	surfaceparm nolightmap
	
	q3map_sun	.5 .6 .8 90 225 55
	q3map_surfacelight 150

	qer_editorimage textures/skies/toxicbluesky.tga

	skyparms - 512 -

	{
		map textures/skies/bluedimclouds.tga
		tcMod scale 3 2
		tcMod scroll 0.15 0.15
	}
	{
		map textures/skies/topclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scale 3 3
		tcMod scroll 0.05 0.05
	}

	//{
	//	map textures/liquids/electric_fog.tga
	//	blendfunc gl_dst_color gl_one
	//	rgbgen wave triangle .8 .2 0 5
	//	tcmod scale -.02 -.02
	//	tcmod rotate 10
	//	tcmod scroll .5 -.5
	//}



}

//*******************************************************
//*              Kevin's pj_arena1 sky with more light
//*******************************************************
textures/skies/kcbasesky_arena1_sky
{
	qer_editorimage textures/skies/pjbasesky.tga
	surfaceparm noimpact
	surfaceparm nolightmap
	q3map_globaltexture
	q3map_lightsubdivide 256 
	q3map_sun	0.266383 0.274632 0.358662 150 60 85
	q3map_surfacelight 500

	skyparms - 512 -
	
	{
		map textures/skies/dimclouds.tga
		tcMod scroll 0.01 0.01
		tcMod scale 3 3
		depthWrite
	}
	{
		map textures/skies/pjbasesky.tga
		blendfunc GL_ONE GL_ONE
		tcMod scroll -0.01 -0.01
		tcMod scale 5 5
	}
}

//*******************************************************
//*              Paul Jaquays' pj_arena1 sky
//*******************************************************
textures/skies/pjbasesky_arena1_sky
{
	qer_editorimage textures/skies/pjbasesky.tga
	surfaceparm noimpact
	surfaceparm nolightmap
	q3map_globaltexture
	q3map_lightsubdivide 256 
	q3map_sun	0.266383 0.274632 0.358662 150 60 85
	q3map_surfacelight 100

	skyparms - 512 -
	
	{
		map textures/skies/dimclouds.tga
		tcMod scroll 0.01 0.01
		tcMod scale 3 3
		depthWrite
	}
	{
		map textures/skies/pjbasesky.tga
		blendfunc GL_ONE GL_ONE
		tcMod scroll -0.01 -0.01
		tcMod scale 5 5
	}
}
//*******************************************************
//*              Paul Jaquays' pj_ctf2 sky
//*******************************************************
textures/skies/pj_ctf2_sky
{
	qer_editorimage textures/skies/pjbasesky.tga
	q3map_lightimage textures/skies/topclouds.tga
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	q3map_globaltexture
	q3map_lightsubdivide 512 
	q3map_sun	0.266383 0.274632 0.358662 250 90 45
	q3map_surfacelight 100

	skyparms full 128 -
	
	{
		map textures/skies/bluedimclouds.tga
		tcMod scroll 0.015 0.016
		tcMod scale 3 3
		depthWrite
	}
	{
		map textures/skies/topclouds.tga
		blendfunc GL_ONE GL_ONE
		tcMod scroll -0.01 -0.012
		tcMod scale 5 5
	}
}

//*******************************************************
//*              Paul Jaquays' pj_arena4 sky
//*******************************************************
textures/skies/pj_arena4_sky
{
	qer_editorimage textures/skies/pjbasesky.tga
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	q3map_globaltexture
	q3map_lightsubdivide 512 
	q3map_sun	0.266383 0.274632 0.358662 200 165 45
	q3map_surfacelight 100

	skyparms full 128 -
	
	{
		map textures/skies/dimclouds.tga
		tcMod scroll 0.015 0.016
		tcMod scale 3 3
		depthWrite
	}
	{
		map textures/skies/pjbasesky.tga
		blendfunc GL_ONE GL_ONE
		tcMod scroll -0.01 -0.012
		tcMod scale 5 5
	}
}
//*******************************************************
//*              Paul Jaquays' pj_ctf1 sky /june 30, 199
//*******************************************************
textures/skies/pj_ctf1sky
{
	qer_editorimage textures/skies/pjbasesky

	surfaceparm noimpact
	surfaceparm nolightmap
	q3map_globaltexture
	q3map_lightsubdivide 512 
	q3map_sun	0.266383 0.274632 0.358662 100 350 75
	q3map_surfacelight 150

	skyparms full 200 -
	
	{
		map textures/skies/dimclouds.tga
		tcMod scroll 0.015 0.016
		tcMod scale 3 3
		depthWrite
	}
	{
		map textures/skies/pjbasesky.tga
		blendfunc GL_ONE GL_ONE
		tcMod scroll -0.01 -0.012
		tcMod scale 5 5
	}
}
//*******************************************************
//*              Paul Jaquays' pj_arena2 sky
//*******************************************************
textures/skies/pj_arena2sky
{
	qer_editorimage textures/skies/pjbasesky

	surfaceparm noimpact
	surfaceparm nolightmap
	q3map_globaltexture
	q3map_lightsubdivide 512 
	q3map_sun	0.266383 0.274632 0.358662 100 350 55
	q3map_surfacelight 100

	skyparms full 200 -
	
	{
		map textures/skies/dimclouds.tga
		tcMod scroll 0.015 0.016
		tcMod scale 3 3
		depthWrite
	}
	{
		map textures/skies/pjbasesky.tga
		blendfunc GL_ONE GL_ONE
		tcMod scroll -0.01 -0.012
		tcMod scale 5 5
	}
}
//*******************************************************
//*              Paul Jaquays' pj_dm9 sky
//*******************************************************
textures/skies/pj_dm9sky
{
	qer_editorimage textures/skies/toxicbluesky.tga
	q3map_lightimage textures/skies/topclouds.tga
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	q3map_globaltexture
	q3map_lightsubdivide 512 

	q3map_sun	.5 .6 .8 140 165 60
	q3map_surfacelight 80

	skyparms - 512 -
	{
		map textures/skies/bluedimclouds.tga
		tcMod scale 3 2
		tcMod scroll 0.15 0.15
		depthWrite
	}
	{
		map textures/skies/topclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scale 3 3
		tcMod scroll 0.05 0.05
	}
}
textures/common/nightsky
{
	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap

	sky env/blue
	cloudparms 192 full

	{
		map textures/bwhtest/tileclouds.tga
		blendfunc GL_ONE GL_ONE
		tcMod scroll 0.02 0.02
		tcMod scale 2 2
	}
}
textures/common/nightsky2
{
	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap

	q3map_surfacelight 30
	sky env/night
	cloudparms 192

	{
		map env/purpleclouds.tga
		blendfunc GL_ONE GL_ONE
		tcMod scroll 0.01 0.01
		tcMod scale 2 2
	}

	{
		map env/clouds.tga
		blendfunc GL_DST_COLOR GL_ZERO
		tcMod scroll 0.05 0.05
		tcMod scale 3 3
	}
}

textures/skies/nightsky_xian_dm4
{
	qer_editorimage textures/skies/nightsky_nocloud.tga
	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap
	q3map_sun 1 1 1 80 -41 58
	q3map_surfacelight 50
	skyparms env/xnight2 - -
}

textures/skies/xnightsky
{
	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	q3map_sun 1 1 1 100 -41 58
	q3map_surfacelight 300

	sky env/xnight2
	cloudparms 512 half

	{
		map env/xnightsky4.tga
		tcMod scroll 0.005 0.002
		tcMod scale 12 18
		blendfunc GL_ONE GL_ONE
	}

	{
		map env/xnightsky3.tga
		tcMod scroll 0.001 0.003
		tcMod scale 6 10
		blendfunc GL_DST_COLOR GL_ZERO
	}
}
textures/skies/xnightsky_600
{
	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap

	q3map_surfacelight 600

	qer_editorimage textures/skies/xnightsky.tga

	sky env/xnight2
	cloudparms 256 half

	{
		map env/xnightsky.tga
		blendfunc GL_ONE GL_ONE
		tcMod scroll 0.010 0.010
		tcMod scale 10 10
	}

	{
		map env/xnightsky1.tga
		blendfunc GL_DST_COLOR GL_ZERO
		tcMod scroll 0.010 0.007
		tcMod scale 10 10
	}
}
textures/skies/hellskyteam
{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	q3map_sun 5 3 6 40 80 70
	q3map_surfacelight 45

	skyparms - 512 -
	//sky env/hell2
	//cloudparms 512 full
	//lightning
	{
		map textures/skies/dimclouds.tga
		tcMod scroll 0.1 0.1
		tcMod scale 3 2
		depthWrite
	}
	{
		map textures/skies/redcloudsa.tga
		blendFunc GL_ONE GL_ONE
		tcMod scroll 0.05 0.05
		tcMod scale 3 3
	}
}
	//{
		//map env/redcloudsa.tga
		//blendfunc GL_ONE GL_ONE
		//tcMod scroll 0.05 0
		//tcMod scale 2 2
		//
	//}
//
	//{
		//map env/redclouds.tga
		//blendfunc GL_ONE GL_SRC_COLOR
		//tcMod scroll 0.05 0.05
		//tcMod scale 1 1
	//}
//}




textures/skies/toxicskydim
//used in tim_dm1
{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	q3map_surfacelight 100
	q3map_sun	1 1 0.5 80 30 70

	qer_editorimage textures/skies/toxicsky.tga
	skyparms - 512 -
	//cloudparms 512 full
	{
		map textures/skies/inteldimclouds.tga
		tcMod scroll 0.1 0.1
		tcMod scale 3 2
		depthWrite
	}
	{
		map textures/skies/intelredclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scroll 0.05 0.05
		tcMod scale 3 3
	}
}
textures/skies/toxicskytim_dm4
//used in tim_dm4
{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	q3map_sun	1 1 0.5 90 120 75
	q3map_surfacelight 80

	qer_editorimage textures/skies/toxicsky.tga

	skyparms - 512 -
	//cloudparms 512 full
	{
		map textures/skies/inteldimclouds.tga
		tcMod scale 3 2
		tcMod scroll 0.15 0.15
		depthWrite
	}
	{
		map textures/skies/intelredclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scale 3 3
		tcMod scroll 0.05 0.05
	}
}


textures/skies/toxicskytim_ctf1
//used in tim_ctf1
{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	q3map_sun	1 1 0.5 90 70 40
	q3map_surfacelight 120

	qer_editorimage textures/skies/toxicsky.tga

	skyparms - 512 -
	//cloudparms 512 full
	{
		map textures/skies/inteldimclouds.tga
		tcMod scale 3 2
		tcMod scroll 0.15 0.15
		depthWrite
	}
	{
		map textures/skies/intelredclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scale 3 3
		tcMod scroll 0.05 0.05
	}
}

textures/skies/toxicskytim_blue
//used in tim_ctf1 blue
{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	q3map_sun	.5 .6 .8 100 70 60
	q3map_surfacelight 120

	qer_editorimage textures/skies/toxicbluesky.tga

	skyparms - 512 -
	//cloudparms 512 full
	{
		map textures/skies/bluedimclouds.tga
		tcMod scale 3 2
		tcMod scroll 0.15 0.15
		depthWrite
	}
	{
		map textures/skies/topclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scale 3 3
		tcMod scroll 0.05 0.05
	}
}

textures/skies/toxicskytim_dm5
//used in tim_dm5

{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	q3map_sun 1 .78 .48 90 225 55


	// old q3map_sun	.5 .6 .8 90 225 55
	q3map_surfacelight 120

	qer_editorimage textures/skies/toxicbluesky.tga

	skyparms - 512 -
	//cloudparms 512 full
	{
		map textures/skies/bluedimclouds.tga
		tcMod scale 3 2
		tcMod scroll 0.15 0.15
		depthWrite
	}
	{
		map textures/skies/topclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scale 3 3
		tcMod scroll 0.05 0.05
	}
}

textures/skies/toxicskytim_dm3
//used in tim_dm3

{
	qer_editorimage textures/skies/toxicsky.tga
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	skyparms 512 full -

	q3map_surfacelight 80
	q3map_sun	1 1 0.5 100	30 60

//	cloudparms 512 full

	skyparms - 512 -

	{
		map textures/skies/inteldimclouds.tga
		tcMod scroll 0.1 0.1
		tcMod scale 3 2
		depthWrite
	}
	{
		map textures/skies/intelredclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scroll 0.05 0.05
		tcMod scale 3 3
	}
}


textures/skies/toxicskytim_dm8
//used in tim_dm8

{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	q3map_sun	1 1 0.5 85 220 40
	q3map_surfacelight 120

	qer_editorimage textures/skies/toxicsky.tga

	skyparms - 512 -
	//cloudparms 512 full
	{
		map textures/skies/inteldimclouds.tga
		tcMod scale 3 2
		tcMod scroll 0.15 0.15
		depthWrite
	}
	{
		map textures/skies/intelredclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scale 3 3
		tcMod scroll 0.05 0.05
	}
}

textures/skies/toxicskytim_dm9
//used in tim_dm9

{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	q3map_sun	.980 .835 .3 100 25 65
	q3map_surfacelight 80

	qer_editorimage textures/skies/toxicsky.tga

	skyparms - 512 -
	//cloudparms 512 full
	{
		map textures/skies/inteldimclouds.tga
		tcMod scale 3 2
		tcMod scroll 0.15 0.15
		depthWrite
	}
	{
		map textures/skies/intelredclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scale 3 3
		tcMod scroll 0.05 0.05
	}
}



textures/skies/toxicskytim_dm2
//used in tim_dm2

{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	q3map_sun	.5 .6 .8 100 190 75
	q3map_surfacelight 120

	qer_editorimage textures/skies/toxicbluesky.tga

	skyparms - 512 -
	//cloudparms 512 full
	{
		map textures/skies/bluedimclouds.tga
		tcMod scale 3 2
		tcMod scroll 0.15 0.15
		depthWrite
	}
	{
		map textures/skies/topclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scale 3 3
		tcMod scroll 0.05 0.05
	}
}

textures/skies/q3dm0
//used in q3dm0

{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	q3map_sun	.5 .6 .8 60 80 75
	q3map_surfacelight 80

	qer_editorimage textures/skies/toxicbluesky.tga

	skyparms - 512 -
	//cloudparms 512 full
	{
		map textures/skies/bluedimclouds.tga
		tcMod scale 3 2
		tcMod scroll 0.15 0.15
		depthWrite
	}
	{
		map textures/skies/topclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scale 3 3
		tcMod scroll 0.05 0.05
	}
}





textures/skies/toxicskytim_dm7
//used in tim_dm7

{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	q3map_sun	.5 .6 .8 100 260 50
	q3map_surfacelight 120

	qer_editorimage textures/skies/toxicbluesky.tga

	skyparms - 512 -
	//cloudparms 512 full
	{
		map textures/skies/bluedimclouds.tga
		tcMod scale 3 2
		tcMod scroll 0.15 0.15
		depthWrite
	}
	{
		map textures/skies/topclouds.tga
		blendFunc GL_ONE GL_ONE
		tcMod scale 3 3
		tcMod scroll 0.05 0.05
	}
}
	
textures/hell/hellsky2goo
{
	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap

	q3map_surfacelight 300

	sky env/hell2
	cloudparms 256

	{
		map env/redcloudsa.tga
		blendfunc GL_ONE GL_ONE
		tcMod scroll 0.05 0
		tcMod scale 2 2
	}

	{
		map env/redclouds.tga
		blendfunc GL_ONE GL_SRC_COLOR
		tcMod scroll 0.05 0.05
		tcMod scale 1 1
	}
}
textures/castle/sky_castle1
{
	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap

	sky env/castle1
	cloudparms 256

	{
		map env/redcloudsa.tga
		blendfunc GL_ONE GL_ONE
		tcMod scroll 0.05 0
		tcMod scale 2 2
	}

	{
		map env/redclouds.tga
		blendfunc GL_DST_COLOR GL_ZERO
		tcMod scroll 0.05 0.05
		tcMod scale 1 1
	}
}



textures/common/sky_30
{
	surfaceparm sky
	surfaceparm noimpact
	q3map_surfacelight 30
	surfaceparm nolightmap
	sky	env/unit1
}
textures/common/sky_100
{
	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap
	q3map_surfacelight 100
	sky	env/unit1
}
textures/common/sky_1000
{
	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap
	q3map_surfacelight 1000
	sky	env/unit1
}
textures/common/sky_150
{
	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap
	q3map_surfacelight 150
	sky	env/unit1
}
textures/common/sky_300
{
	surfaceparm sky
	surfaceparm noimpact
	q3map_surfacelight 300
	surfaceparm nolightmap
	sky	env/unit1
}
textures/common/sky_500
{
	surfaceparm sky
	surfaceparm noimpact
	q3map_surfacelight 500
	surfaceparm nolightmap
	sky	env/unit1
}
textures/common/sky_space
{
	surfaceparm sky
	surfaceparm noimpact
	q3map_surfacelight 50
	surfaceparm nolightmap
	sky	env/space1
}
textures/common/sky_u2
{
	surfaceparm sky
	surfaceparm noimpact
	q3map_surfacelight 50
	surfaceparm nolightmap
	sky	env/unit2
}
textures/common/sky1
{
	surfaceparm sky
	surfaceparm noimpact
	q3map_surfacelight 50
	surfaceparm nolightmap
	sky	env/unit1
}

textures/skies/mkc_dm4sky
//used in mkc_dm4
{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	q3map_sun	0.9 0.8 1 25 135 60
	q3map_surfacelight 80

	qer_editorimage textures/skies/dimclouds.tga

	skyparms - 512 -
	//cloudparms 512 full
	{
		map textures/skies/nightsky.tga
		tcMod scale 5 8
		tcMod scroll 0.7 0.1
		depthWrite
	}
	{
		map textures/skies/redclouds.tga
		blendFunc GL_DST_COLOR GL_ZERO
		tcMod scale 3 5
		tcMod scroll 0.3 0.1
	}
}

textures/skies/mkc_ctf5sky
//used in mkc_ctf5
{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	q3map_sun	0.9 0.8 1.0 70 45 60
	q3map_surfacelight 50

	qer_editorimage textures/skies/dimclouds.tga

	skyparms - 512 -
	//cloudparms 512 full
	{
		map textures/skies/nightsky.tga
		tcMod scale 5 8
		tcMod scroll 0.7 0.1
		depthWrite
	}
	{
		map textures/skies/nightsky_nocloud.tga
		blendFunc GL_DST_COLOR GL_ZERO
		tcMod scale 3 5
		tcMod scroll 0.3 0.1
	}
}


textures/skies/proto_sky
{
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	skyparms 512 full -

	q3map_surfacelight 30
	q3map_sun .473 .243 .314 30 35 78

//	cloudparms 512 full

	skyparms - 512 -

	{
		map textures/skies/proto_sky.tga
		tcMod scroll .02 .04
		tcMod scale 3 4
		depthWrite
	}
	{
		map textures/skies/proto_sky2.tga
		blendFunc GL_ONE GL_ONE
		tcMod scroll 0.005 0.005
                //rgbgen wave triangle 1 2 0 7
		tcMod scale 3 3
	}
        
     
}


