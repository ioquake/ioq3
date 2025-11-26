//	*************************************************
//	* 		TEST SHADERS			*
//	*        DO NOT USE IN PROJECT MAPS		*
//	*************************************************

textures/vector
// testing tcgen vector
{
	nomipmaps
    {
			map textures/gothic_block/blocks10.tga
				tcGen vector ( 0.0001 0.0001 0.0001 ) ( 0 0 0 )
                tcMod scroll 0.01 0
                rgbGen identity
	}
}


textures/sfx/kenelectric
{    
         {
                map textures/sfx/kenelectric.tga
                blendFunc GL_ONE GL_ZERO
                tcmod scale 7 7
                tcMod scroll 5 -5
                tcmod rotate 360
                rgbGen identity
	}
        {
		map textures/effects/tinfx2.tga
                tcGen environment
                blendFunc GL_ONE GL_ONE              
                //rgbGen lightingDiffuse
	}   
        //{
		//map models/players/doom/phobos.tga
             //   blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		 //rgbGen lightingDiffuse
	//}
}

// Rogues sorting trimmed glass//

textures/test/rnd_ledm1
{
        surfaceparm nolightmap
        surfaceparm trans
        qer_trans 0.5
        qer_editorimage textures/test/rnd_ledm1.tga
        
        { 
                map textures/test/rnd_ledm1.tga
                blendFunc GL_ONE GL_ONE
                rgbGen identity
        }
        {
                map textures/test/rnd_ledm2.tga
                blendFunc GL_ZERO GL_SRC_COLOR
                rgbGen identity 
        }
}



	

textures/test/electricity3
	{
		//	*************************************************
		//	* Ripply Water Feb 16 1999			*
		//	* IF YOU CHANGE THIS PLEASE COMMENT THE CHANGE	*
		//	*************************************************

		qer_trans .5
	
		surfaceparm trans
		surfaceparm nonsolid
		surfaceparm water
		surfaceparm nolightmap

		cull disable
		tesssize 64
		deformVertexes wave 100 triangle 2 2 1 0.1
		


		{
			map textures/test/fub.tga
			blendFunc GL_ZERO GL_ONE
			alphafunc GT0
			tcMod scale .1 .01
			tcmod turb 0 .01 .3 .1
			alphaGen wave sin .5 .5 0 .01
			tcMod scroll .1 .1
			depthWrite
		}
		{
			map textures/liquids/pool2.tga
			blendfunc GL_ONE GL_SRC_COLOR	
			tcMod scale .03 .03
			tcMod scroll .001 .001
		}
	

		
		{
			map textures/test/electricity9.tga
			blendfunc GL_DST_COLOR GL_ONE
			tcmod turb 0 .01 .3 10
			tcMod scale .5 .5	
			tcMod rotate 1
			depthfunc equal
			tcMod scroll -.01 -.05
		}

		{
			map textures/test/electricity9.tga
			blendfunc GL_DST_COLOR GL_ONE
			tcmod turb 0 .01 .3 10
			tcMod scale .5 .5	
			tcMod rotate 1
			depthfunc equal
			tcMod scroll -.01 -.05
		}

		

			

		//	END
	}


textures/gothic_trim/hellodolly
{

	//	*************************************************
	//	*      	TEST BORDER WALL			*
	//	*      	Feb 24 1999				*	
	//	*	Please Comment Changes			*
	//	*************************************************

		qer_editorimage	textures/gothic_trim/metalbase13_d.tga	

		{
			map textures/gothic_block/blocks10.tga
			rgbGen identity
			// Name of background wall
		}


		{	
			map textures/gothic_trim/metalbase13_d.tga
			// Name of border wall
			blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		}

		{	
			rgbGen identity
			map $lightmap
			blendfunc GL_DST_COLOR GL_ZERO
		}	

		
			

	//	END
}

textures/test/blocks10pent
{

	//	*************************************************
	//	*      	TEST BORDER WALL			*
	//	*      	Feb 24 1999				*	
	//	*	Please Comment Changes			*
	//	*************************************************

		q3map_surfacelight 1000
		surfaceparm nodamage
		qer_lightimage textures/test/jumpadsmall.tga	

	
	{
		map textures/sfx/bouncepad01block18b.tga
		rgbGen identity
	}
	
	{
		map $lightmap
		rgbGen identity
		blendfunc gl_dst_color gl_zero
	}
	
	{
		map textures/sfx/bouncepad01b_layer1.tga
		blendfunc gl_one gl_one
		rgbGen wave sin .5 .5 0 1.5	
	}

	{
		clampmap textures/test/jumppadsmall.tga
		blendfunc gl_one gl_one
		tcMod stretch sin .6 .4 0 1.5
		rgbGen wave square .5 .5 .25 1.5
	}


		
			

	//	END
}



textures/dont_use/foggentest_shader
{
//	*************************************************
//	* Kevin's fogggen test				*
//	*************************************************

	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm fog
	surfaceparm nolightmap

	fogparms ( .2 .1 .3 ) 0 0
	foggen sin .1 .1 .1 .1

	cull backsided


	
//	END
}

textures/dont_use/sign_test_glow
{
	surfaceparm trans
	surfaceparm nolightmap
	q3map_surfacelight 100
	
	{
		map textures/dont_use/sign_test.tga
		alphaMap $fromBlack
		blendFunc GL_ONE GL_ONE
	}
	
	{	
		map textures/dont_use/sign_test_glow.tga
		alphaMap $fromBlack
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .5 .5 1 .1
	}

//	END
}
	

textures/dont_use/nightsky_kc
{

//	*************************************************
//	* Kevin's strange test sky		*
//	*************************************************
	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap
	q3map_surfacelight 200

	cloudparms 256 half

	


	
	{
		map textures/dont_use/strangesky3_kc.tga
		tcMod scroll  .03 .03
	}


	{
		map textures/dont_use/strangesky.tga
		blendfunc GL_ONE GL_SRC_COLOR
		tcMod scroll  .05 .05	
	}

	
	{
		map textures/dont_use/strangesky2_kc.tga
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		tcMod scroll 0.01 0.01
		tcMod scale .5 .5
	}
	
//END
}

textures/dont_use/nightsky_kcold
{

//	*************************************************
//	* Kevin's four layer storm sky			*
//	*************************************************
	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap
	q3map_surfacelight 200

	cloudparms 256 full
	lightning


	
	{
		map textures/dont_use/tileclouds3_kc.tga
		tcMod scroll 0.01 0.01
		tcMod scale 2 2
	}

	
	{
		map textures/dont_use/tileclouds4_kc.tga
		blendFunc GL_ONE GL_ONE
		tcMod scroll  .04 .05
	}


	{
		map textures/dont_use/lightningsky8_kc.tga
		blendfunc GL_DST_COLOR GL_ONE
		tcMod scroll .1 .1
		tcMod scale 4 4
	}

	{
		map textures/dont_use/tileclouds3b_kc.tga
		blendFunc GL_DST_COLOR GL_ONE
		tcMod scroll  0.01 .02
	}

	

	
//END
}

textures/dont_use/foglightshader
{
//	*************************************************
//	* Kevin's simplified fog			*
//	*************************************************

	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm fog
	surfaceparm nolightmap

	cull backsided
	{
	map textures/dont_use/fogkc.tga
	blendFunc GL_ONE GL_ONE
	tcMod scroll 1 1
	tcMod scale .25 .25
	tcMod turb .1 .1 .1 .1
	}

	
//	END
}



textures/dont_use/train1kc_shader
{
//	*************************************************
//	* test shader wall				*
//	*************************************************


	{
	map $lightmap
		rgbGen identity
	}
	
	{
	map textures/common/pjrock5b.tga
	blendFunc GL_DST_COLOR GL_SRC_COLOR 
		rgbGen identity
	detail
	tcmod scale 9.1  9.2
	}
	
	{
	map textures/common/pjrock5.tga
	blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}

	


		
//	END
}

textures/dont_use/flameshader
{
//	*************************************************
//	* test flames					*
//	*************************************************
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm fog
	surfaceparm nolightmap

	cull none
	fogparms ( .3 .1 .1 ) 0 32
	tesssize 16
	deformVertexes wave 100 sin 2.5 5 10 .1
		
//	END
}

textures/dont_use/openwindow
{	
	
	surfaceparm nolightmap
	cull none
	
	{
	map textures/dont_use/openwindow.tga
	alphaMap openwindow.tga
	blendFunc GL_ONE_MINUS_SRC_ALPHA GL_SRC_ALPHA
	depthWrite
	}
}

textures/dont_use/web
{	
	
	surfaceparm nolightmap
	cull twosided
	{
	map textures/dont_use/web.tga
	alphaMap $fromBlack
	blendFunc GL_ONE_MINUS_SRC_ALPHA GL_SRC_ALPHA
	depthWrite
	}
}

textures/dont_use/metal2_2kc
{	
	
	surfaceparm nolightmap
	cull none
	{
	map textures/dont_use/metal2_2kc.tga
	alphaMap $fromBlack
	blendFunc GL_ONE_MINUS_SRC_ALPHA GL_SRC_ALPHA
	depthWrite
	}
}

textures/dont_use/rustysupport
{	
	
	surfaceparm nolightmap
	cull none
	{
	map textures/dont_use/rustysupport.tga
	alphaMap $fromBlack
	blendFunc GL_ONE_MINUS_SRC_ALPHA GL_SRC_ALPHA
	depthWrite
	}
}

textures/dont_use/sky_test
{
	surfaceparm sky
	surfaceparm noimpact
	q3map_surfacelight 100
	surfaceparm nolightmap
	sky	textures/dont_use/sky_test
}



textures/dont_use/shader2
{
//	*************************************************
//	* test shader2 for test use			*
//	*************************************************
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nolightmap
	{
	map textures/dont_use/shader2.tga
	blendfunc GL_ONE GL_ONE
	tcMod turb .2 .01 0 .1
	tcMod scroll  0 0
	}
//	END
}


textures/dont_use/shader3
{
//	*************************************************
//	* test shader3 for test use			*
//	*************************************************
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nolightmap
	
	tesssize 64
	deformVertexes wave 100 sin 2 2 10 .01
	
	{
	map textures/dont_use/blood2.tga
	tcMod turb .1 .1 0 0.02
	tcMod scroll  .1 .1
	}


//	END
}


textures/dont_use/shader4
{
//	*************************************************
//	* test shader4 for test use			*
//	*************************************************
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nolightmap
	tesssize 64
	deformVertexes wave 100 sin 1 1 1 .1

	q3map_surfacelight 100
	
	{
	map textures/dont_use/slimetest7c.tga
	blendfunc GL_ONE_MINUS_DST_COLOR GL_ZERO
	tcMod turb .01 .2 10 0.01
	tcMod scroll  .2 .1
	}
	{
	map textures/dont_use/bubbleslimeelectricc.tga
	blendfunc GL_DST_COLOR GL_ONE
	tcMod turb .5 .5 10 0.01
	tcMod scroll .2 .2
	tcMod scale .1 .1
	}
	
	
	
	
//	END
}


textures/dont_use/shader5
{
//	*************************************************
//	* test shader5 for test use			*
//	*************************************************
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nolightmap
	tesssize 64
	
	{
	map textures/dont_use/firetestb.tga
	blendfunc gl_one gl_one
	tcMod scroll 5 15
	tcMod scale .25 .25
	tcMod turb .2 .2 .1 1
	}

	{
	map textures/dont_use/firetest.tga
	blendFunc GL_dst_color gl_src_alpha
	tcMod scroll 0 10
	tcMod scale .1 .1
	tcMod turb .2 .1 .1 1
	rgbGen wave SIN .5 .5 1 .1
	}
	
	{
	map textures/dont_use/firetest2.tga
	blendFunc GL_dst_color gl_one
	tcMod scroll  10 50
	tcMod turb .1 .1 .5 .5
	rgbGen wave SIN .5 .5 1 .1
	}
	
		
//	END
}


textures/dont_use/shader5old
{
//	*************************************************
//	* test shader5 for test use			*
//	*************************************************
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nolightmap
	tesssize 64
	
	
	{
	map textures/dont_use/firetestb.tga
	blendfunc gl_one gl_one
	tcMod scroll 5 15
	tcMod scale .25 .25
	tcMod turb .2 .2 .1 1
	}

	{
	map textures/dont_use/firetest.tga
	blendFunc GL_dst_color gl_src_alpha
	tcMod scroll 0 10
	tcMod scale .1 .1
	tcMod turb .2 .1 .1 1
	}
	
	{
	map textures/dont_use/firetest2.tga
	blendFunc GL_dst_color gl_one
	tcMod scroll  10 50
	tcMod turb .1 .1 .5 .5
	}

//	END
}



textures/test/slimeshader
{
//	*************************************************
//	* Kevin's temp slime with electricity		*
//	*************************************************

	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm slime
	surfaceparm nolightmap

	tesssize 64
	cull disable
	deformVertexes wave 100 sin .5 .5 1 .05

	q3map_surfacelight 500

	{
	map textures/test/slimetest6.tga
	tcMod turb .25 0.1 1 0.02
	tcMod scroll  .2 .2
	}
	{
	map textures/test/slimecrust5d.tga
	blendfunc GL_DST_COLOR GL_SRC_ALPHA
	tcMod turb .5 .5 1 .02
	tcMod scroll .1 .01
	tcMod scale .1 .1
	}
	{
	map textures/test/electricslime.tga
	tcMod turb 1 1 1 .01
	tcMod scroll 1 5
	tcMod scale .5 .5
	blendfunc GL_ONE GL_ONE
	rgbGen wave sin .5 .5 0 .1
	}
	{
	map textures/test/slimecrust5d.tga
	blendfunc GL_ONE GL_SRC_ALPHA
	tcMod turb .2 .1 5 .01
	tcMod scroll  -.01 -.01
	}
	
//	FOG
	surfaceparm fog
	fogparms ( .2 1 .25 ) 0 256

//	END
}



textures/test/fogtestshader
{
//	*************************************************
//	* Kevin's simplified fog			*
//	*************************************************

	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm fog
	surfaceparm nolightmap

	cull disable
	fogparms ( .2 .225 .25 ) 0 0
	tesssize 64
	
	{
	map textures/test/cloud2.tga
	blendFunc GL_DST_COLOR GL_ONE
	tcGen environment
	tcMod turb .5 .5 .5 0.03
	tcMod scroll .25 .25
	tcMod scale .5 .5
	}
	
//	END
}




textures/test/lavacrustshader
{
//	*************************************************
//	* Kevin's brown lava crust			*
//	*************************************************

	surfaceparm trans
	//surfaceparm nonsolid
	//surfaceparm noimpact
	surfaceparm lava
	surfaceparm nolightmap

	
	
	light 1
	q3map_surfacelight 100
	
	tesssize 64
	deformVertexes wave 100 sin 1 1 0 .03
	cull disable

	{
	map textures/test/lavatest.tga
	tcMod turb .25 0.1 1 0.02
	tcMod scroll  .2 .2
	}
	{
	map textures/test/lavacrust.tga
	blendfunc GL_DST_COLOR GL_SRC_ALPHA
	tcMod turb .5 .5 5 0.01
	tcMod scroll .1 .1
	tcMod scale .1 .1
	rgbGen wave triangle 1 1 2 1
	}
	{
	map textures/test/lavacrust.tga
	blendfunc GL_ONE GL_SRC_ALPHA
	tcMod turb .2 .01 5 .01
	tcMod scroll  -.01 -.01
	}
	
	
//	END
}


textures/test/water_trans
{
//	**************************************************
//	* Kevin's simplified version of translucent water*
//	* Should be used for small pools		 *
//	**************************************************

	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm water
	surfaceparm nolightmap

	cull disable
	tesssize 64
	deformVertexes wave 100 sin 1 1 1 .1
	
	
	{
	map textures/test/watertest8d.tga
	blendfunc GL_ONE GL_SRC_COLOR	
	tcMod scale .03 .03
	tcMod scroll .001 .001
	}
	
	{
	map textures/test/watertest12e.tga
	blendfunc GL_DST_COLOR GL_ONE
	tcMod scale .2 .2
	tcMod stretch sin 1 .1 .02 .05
	rgbGen wave sin .8 .2 .02 .05
	tcMod scroll -.001 .001
	}
	
	{
	map textures/test/watertest12e7.tga
	blendfunc GL_DST_COLOR GL_ONE
	tcMod turb .1 .1 0 .01
	tcMod scale .5 .5
	tcMod scroll -.025 .02
	}
	
	{
	map textures/test/watertest12e6.tga
	blendfunc GL_DST_COLOR GL_ONE
	tcMod scale .3 .3
	tcMod turb .1 .01 .01 .01
	tcMod stretch sin 1 .3 .015 .015
	rgbGen wave sin .7 .3 .02 .02
	tcMod scroll .001 -.001
	}

	



	

	
	
	
	
	

//	END
}



textures/test/watertile
{	
//	*************************************************
//	* Kevin's test water tile with shader		*
//	*************************************************
	
	cull disable
	
	{
	map textures/paulbase/tilefloor2.tga
	}
	//{
	//map textures/test/watertest7f.tga
	//blendFunc GL_ONE GL_SRC_COLOR
	//tcMod turb .01 .02 0 0.05
	//tcMod scroll .01 .01
	//}
	//{
	//map textures/test/watertest7h.tga
	//blendFunc GL_ONE GL_SRC_COLOR
	//tcMod turb .01 .03 0 0.05
	//tcMod scroll -.01 -.01
	//}
	
//	END
}


textures/test/river
{	
//	*************************************************
//	* Kevin's test river shader 			*
//	*************************************************
	
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm water
	surfaceparm nolightmap

	cull disable
	tesssize 64
	deformVertexes wave 100 sin 1 1 1 .05

	{
	map textures/test/watertest8c.tga
	blendFunc GL_DST_COLOR GL_SRC_COLOR
	tcMod turb .01 .01 0 0.1
	tcMod scroll -5 0
	tcMod scale .5 .5
	}
	{
	map textures/test/watertest10f.tga
	blendfunc GL_DST_COLOR GL_SRC_COLOR
	tcMod turb .01 .01 0 0.1
	rgbGen wave sin .8 .01 1 1	
	tcMod scroll -6 0
	tcMod scale .5 .5
	}
	{
	map textures/test/watertest8e.tga
	blendFunc GL_DST_COLOR GL_ONE
	tcMod turb .01 .01 .1 .1
	tcMod scroll  -10 0
	}
	
//	END
}

textures/test/waterfall
{	
//	*************************************************
//	* Kevin's test waterfall shader			*
//	*************************************************
	
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm water
	surfaceparm nolightmap

	cull disable
	tesssize 16
	deformVertexes wave 100 sin 0 2 1 .1
	

	{
	map textures/test/watertest8e.tga
	blendFunc GL_ONE GL_SRC_ALPHA
	tcMod turb .01 .01 0 0.1
	tcMod scroll  15 0
	}
	{
	map textures/test/watertest10f.tga
	blendfunc GL_DST_COLOR GL_SRC_COLOR
	tcMod turb .01 .01 0 0.1
	rgbGen wave sin .8 .01 1 1	
	tcMod scroll  20 0
	}
	{
	map  textures/test/watertest10c.tga
	blendFunc GL_ONE GL_SRC_ALPHA
	//tcMod turb .05 .05 .1 0.1
	tcMod scroll 17 1
	tcMod scale .5 .5
	}
	
	
//	END
}

textures/test/lavatest
{
//	*************************************************
//	* Kevin's simplified version of lava		*
//	*************************************************

	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm noimpact
	surfaceparm lava
	surfaceparm nolightmap
	
	light 1
	q3map_surfacelight 100
	
	tesssize 128
	cull disable
	deformVertexes wave 100 sin 5 5 .5 0.02
	
	{
	map textures/test/lavatest.tga
	tcMod turb .25 0.2 1 0.02
	tcMod scroll  .25 .25
	}
	
//	END
}

textures/test/densewaterfog
{
	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm	nolightmap
	backsided
	fogparms ( 0.7 0.7 0.7 ) 128 64
}


textures/test/fire2	
{
	
	{
	map textures/test/fire2.tga
	}
	{
	map textures/test/fire3.tga
	tcMod turb 0 .1 0 .7
	tcMod scroll  0 10
	rgbGen wave sin 1 2 0 1
	}
	{
	map textures/test/fire3.blend.tga
	blendfunc GL_ONE GL_ONE 	
	tcMod scroll  0 10
	}
}

textures/bwhtest/lightning
{
	cull disable
	{
	map textures/bwhtest/lightning.tga
	blendfunc GL_ONE GL_ONE
	tcMod turb 0 0.1 0 1
	}
}

textures/base/bluemet1
{
	{
	map $lightmap
		rgbGen identity
	}
	{
	map textures/base/bluemet1.tga
	blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/bwhtest/crackedearth
{
	{
	map textures/bwhtest/crackedearth.tga
	}
}

textures/test/wispy
{
	surfaceparm nolightmap
	surfaceparm nonsolid
	surfaceparm trans

	deformVertexes wave 100 sin 0 3 0 0.1
	tesssize 64

	{
	map env/alphaclouds.tga
	blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	tcMod scroll 1 0.8
	tcMod scale 0.1 0.1
	tcMod turb 0 0.3 0 0.1
	}
	
}

textures/common/transwater
{
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm water
	surfaceparm nolightmap

	cull disable
	
	tesssize 34
	deformVertexes wave 100 sin 0 2 0 0.1
	{
	map textures/common/water1.tga
	blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/skies/mkc_sky
{
	qer_editorimage textures/skies/pjbasesky.tga
	surfaceparm noimpact
	surfaceparm nolightmap

	q3map_sun	0.266383 0.274632 0.358662 150 60 80
	q3map_surfacelight 700

	skyparms half 256 -
	
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

textures/sfx/mkc_fog_dm1
{
qer_editorimage textures/sfx/fog_timdm1.tga
surfaceparm	trans
surfaceparm	nonsolid
surfaceparm	fog
surfaceparm	nolightmap

cull back
fogparms ( 0 0 0 ) 400 1
}

textures/sfx/mkc_fog_dm1a
{
qer_editorimage textures/sfx/fog_timdm1.tga
surfaceparm	trans
surfaceparm	nonsolid
surfaceparm	fog
surfaceparm	nolightmap

cull back
fogparms ( 0 0 0 ) 400 0
}

textures/sfx/mkc_fog_dm4
{
qer_editorimage textures/sfx/fog_timdm1.tga
surfaceparm	trans
surfaceparm	nonsolid
surfaceparm	fog
surfaceparm	nolightmap

cull back
fogparms ( 64 0 0 ) 256 1
}
