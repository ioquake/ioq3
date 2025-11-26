
textures/sfx/blocks17g_jumpad
{
	surfaceparm nodamage
	q3map_surfacelight 400

	
	{
		map textures/sfx/blocks17g_jumpad.tga
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
		clampmap textures/sfx/jumppadsmall.tga
		blendfunc gl_one gl_one
		tcMod stretch sin 1.2 .8 0 1.5
		rgbGen wave square .5 .5 .25 1.5
	}

	//	END
}


textures/sfx/bullseye
{ 
        cull disable
        //surfaceparm trans	
        surfaceparm nomarks	
        surfaceparm nolightmap 
        deformVertexes autosprite
        //nomipmaps

        {
		clampmap textures/sfx/bullseye.tga
                tcMod stretch sin .8 0.2 0 .2
                tcmod rotate 200
                blendFunc add
                rgbGen identity
	}
        {
	        clampmap textures/gothic_block/blocks18cgeomtrn2.tga
	        tcMod stretch sin .8 0.2 0 .3
                tcmod rotate 70
                alphaFunc GE128 
                rgbGen identity
	}
        {
	        //clampmap textures/sfx/bullseye.tga
                clampmap textures/gothic_block/blocks18cgeomtrn2.tga
                tcMod stretch sin .8 0.2 0 .2
                tcmod rotate 50
                alphaFunc GE128
                rgbGen identity
	}
        {
	        clampmap textures/gothic_block/blocks18cgeomtrn2.tga
                tcMod stretch sin .8 0.2 0 .1
                tcmod rotate 30
                alphaFunc GE128
                rgbGen identity
	}

}
textures/sfx/omm
{ 
    
        surfaceparm trans	
	surfaceparm alphashadow
        surfaceparm nomarks	
	cull none

       {
                map textures/sfx/omm.tga
                blendFunc GL_ONE GL_ZERO
                alphaFunc GE128
                depthWrite
		rgbGen identity
       }
       {
		map $lightmap
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
                depthFunc equal
       }
}
textures/sfx/dust_puppy2
{ 
    
        surfaceparm trans	
	surfaceparm alphashadow
        surfaceparm nomarks	
	cull none

       {
                map textures/sfx/dust_puppy2.tga
                blendFunc GL_ONE GL_ZERO
                alphaFunc GE128
                depthWrite
		rgbGen identity
       }
       {
		map $lightmap
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
                depthFunc equal
       }
}
textures/sfx/spawn_floord2c
{

        {
		map textures/sfx/proto_zzztblu.tga
                tcmod scroll 0 1
                tcMod turb 0 .25 0 5.6
                tcmod scale 2 2
                blendFunc GL_ONE GL_ZERO
                rgbGen identity
	}
        {
	        clampmap textures/gothic_block/blocks18cgeomtrn2.tga
                blendFunc blend
                tcmod rotate 130
                tcMod stretch sin .8 0.2 0 .2
	        rgbGen identity
	}
        {
	        clampmap textures/gothic_block/blocks18cgeomtrn2.tga
		blendFunc blend
                tcmod rotate 80
                tcMod stretch sin .8 0.2 0 .1
	        rgbGen identity
	}
	{
	        map textures/sfx/spawn_floord2c.tga
		blendFunc blend
	        rgbGen identity
	}
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
}
textures/sfx/spawn_floor
{

        {
		map textures/sfx/firegorre2.tga
                tcmod scroll 0 1
                tcMod turb 0 .25 0 1.6
                tcmod scale 2 2
                blendFunc GL_ONE GL_ZERO
                rgbGen identity
	}
        {
	        clampmap textures/gothic_block/blocks18cgeomtrn2.tga
                blendFunc blend
                tcmod rotate 130
                tcMod stretch sin .8 0.2 0 .2
	        rgbGen identity
	}
        {
	        clampmap textures/gothic_block/blocks18cgeomtrn2.tga
		blendFunc blend
                tcmod rotate 80
                tcMod stretch sin .8 0.2 0 .1
	        rgbGen identity
	}
	{
	        map textures/sfx/spawn_floor.tga
		blendFunc blend
	        rgbGen identity
	}
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
}
textures/sfx/fan3blade
{
      cull disable
      surfaceparm trans	

        {
	        clampmap textures/sfx/fan3blade.tga
                alphaFunc GE128
                tcmod rotate 90
	        rgbGen identity
	}
        {
		map $lightmap
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
		depthFunc equal
	}
}
textures/sfx/fan3bladeb
{
      cull disable
      surfaceparm trans	
      surfaceparm nolightmap
      sort 5
        {
	        clampmap textures/sfx/fan3bladeb.tga
                blendFunc blend
                tcmod rotate 999
	        rgbGen identity
	}
        
}
textures/sfx/fan3
{
       cull disable
       surfaceparm alphashadow	
       sort 6
        {
	        map textures/sfx/fan3.tga
		blendFunc blend
	        rgbGen identity
	}
        
        {
		map $lightmap
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/sfx/lavabeam
{
        surfaceparm trans	
        surfaceparm nomarks	
        surfaceparm nonsolid
	surfaceparm nolightmap
	cull none
        nomipmaps
	{
		map textures/sfx/lavabeam.tga
                tcMod Scroll 999 0
                blendFunc add
        }
     
}
textures/sfx/healthfloor
{

        {
		map textures/sfx/proto_zzztblu2.tga
                tcmod scroll 0 1
                tcMod turb 0 .25 0 1.6
                tcmod scale 2 2
                blendFunc GL_ONE GL_ZERO
                rgbGen identity
	}
        {
	        clampmap textures/gothic_block/blocks18cgeomtrn2.tga
                blendFunc blend
                tcmod rotate 130
                tcMod stretch sin .8 0.2 0 .2
	        rgbGen identity
	}
        {
	        clampmap textures/gothic_block/blocks18cgeomtrn2.tga
		blendFunc blend
                tcmod rotate 80
                tcMod stretch sin .8 0.2 0 .1
	        rgbGen identity
	}
	{
	        clampmap textures/sfx/healthfloor.tga
		blendFunc blend
	        rgbGen identity
	}
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
}
textures/sfx/proto_zzztblu3
{
	surfaceparm nolightmap
	surfaceparm nonsolid
	cull twosided
	{
		map textures/sfx/proto_zzztblu3.tga
		tcGen environment
                tcMod turb 0 0.25 0 0.5
                tcmod scroll 1 1
		blendfunc GL_ONE GL_ONE
	}
}

textures/sfx/flametest
{
        deformVertexes autoSprite2
        surfaceparm trans	
        surfaceparm nomarks	
	surfaceparm nolightmap
        q3map_surfacelight 150
        cull none

	{
	        map textures/sfx/flametest.tga
                blendFunc GL_ONE GL_ONE
                rgbgen identity
	}
       
       
}
textures/sfx/bouncepad01_xarch
{

	//q3map_surfacelight 2000
	surfaceparm nodamage
	q3map_lightimage textures/sfx/jumppadsmall.tga	
	q3map_surfacelight 400

	
	{
		map textures/sfx/bouncepad01_xarch.tga
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
		clampmap textures/sfx/jumppadsmall.tga
		blendfunc gl_one gl_one
		tcMod stretch sin 1.2 .8 0 1.5
		rgbGen wave square .5 .5 .25 1.5
	}

	//	END
}


textures/sfx/rain
{
        surfaceparm trans	
        surfaceparm nomarks	
        surfaceparm nonsolid
	surfaceparm nolightmap
        qer_trans .5
        deformVertexes move 3 1 0  sin 0 5 0 0.2
        deformVertexes move .6 3.3 0  sin 0 5 0 0.4
        deformVertexes wave 30 sin 0 10 0 .2
	cull none
        //nopicmip
       
	{
		map textures/sfx/rain.tga
                tcMod Scroll .5 -8
                tcMod turb .1 .25 0 -.1
                blendFunc GL_ONE GL_ONE
        }
        {
		map textures/sfx/rain.tga
                tcMod Scroll .01 -6.3
               
                blendFunc GL_ONE GL_ONE
        }
       
     
}
textures/sfx/largerblock3b3x128_pentred
{
	qer_editorimage textures/sfx/largerblock3b3x128_pent.tga
	q3map_lightimage textures/gothic_floor/pent_glow.tga
	q3map_surfacelight 500

	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/sfx/largerblock3b3x128_pent.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/gothic_floor/pent_glow.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .5 .5 0 0.5
	}
}

textures/sfx/largerblock3b3x128_pent
{
	q3map_lightimage textures/sfx/pentagramfloor_blue3glowtest.tga
	q3map_surfacelight 100
	surfaceparm nomarks
	
	{
		map $lightmap
		rgbgen identity
	}

	{
		map textures/sfx/largerblock3b3x128_pent.tga
		blendfunc gl_dst_color gl_zero
		rgbgen identity
	}

	{
		map textures/sfx/pentagramfloor_blue3glowtest.tga
		blendfunc gl_one gl_one
		rgbgen wave sin .9 .1 0 5
	}

}


textures/sfx/smallerblock3b3dim_pent
{
	qer_editorimage textures/sfx/smallerblock3b3dim_pent.tga
	{
		map textures/sfx/smallerblock3b3dim_pent.tga
		rgbGen identity
	}
	{
		map $lightmap
		rgbGen identity
		blendfunc gl_dst_color gl_zero
	}
	{
		map textures/sfx/pentagramfloor_red_glow.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .5 .5 0 .5	
		depthfunc equal
	}
}

textures/sfx/fishy
{
    tesssize 32
    deformVertexes wave 10 sin 0 3 0 .5  
    cull disable
    surfaceparm nolightmap
    surfaceparm nomarks	
    
        {
                clampmap textures/sfx/fishy.tga
                alphaFunc GE128
                tcMod stretch sin .8 0.10 0 .07
		depthWrite
		
        }


}




textures/sfx/lightmap
{
	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm 	nodrop
	surfaceparm	fog
	fogparms ( .7 .1 .1 ) 64

	
	
		{
			map $lightmap
			blendFunc GL_dst_color GL_one
			tcmod scale 1 .01
			tcMod scroll 1 -2		
		}

	

	
}


textures/sfx/hellfog_pj_dm10
//**************************************
//New death fog that must be in all maps with death fog
//**************************************
{
	qer_editorimage textures/sfx/hellfog.tga
	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm 	nodrop
	surfaceparm 	nolightmap
	q3map_globaltexture
	q3map_surfacelight 50
	fogparms ( .5 .12 .1 ) 392

	
	
	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale -.05 -.05
		tcmod scroll .01 -.01
		rgbgen identity
	}

	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale .05 .05
		tcmod scroll .01 -.01
		rgbgen identity
	}

}

textures/sfx/hellfog_mini_dm10
//**************************************
//New death fog that must be in all maps with death fog
//Change:  EXTREMELY SHALLOW FOG
//**************************************
{
	qer_editorimage textures/sfx/hellfog.tga
	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm 	nodrop
	surfaceparm 	nolightmap
	q3map_globaltexture
	q3map_surfacelight 50
	fogparms ( .5 .12 .1 ) 16

	
	
	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale -.05 -.05
		tcmod scroll .01 -.01
		rgbgen identity
	}

	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale .05 .05
		tcmod scroll .01 -.01
		rgbgen identity
	}

}


textures/sfx/metalfloor_wall_14b_glow
{
	qer_editorimage textures/sfx/metalfloor_wall_14b.tga
	q3map_lightimage textures/sfx/metalfloor_wall_14bglow.tga
	q3map_surfacelight 100

	{
		map $lightmap
		rgbgen identity
	}

	{
		map textures/sfx/metalfloor_wall_14b.tga
		blendfunc gl_dst_color gl_zero
		rgbgen identity
	}

	{
		map textures/sfx/metalfloor_wall_14bglow.tga
		blendfunc gl_one gl_one
		rgbgen wave sin .5 .5 1 0.1
	}

}

textures/sfx/metalfloor_wall_15b_glow
{
	qer_editorimage textures/sfx/metalfloor_wall_15b.tga
	q3map_lightimage textures/sfx/metalfloor_wall_15bglow.tga
	q3map_surfacelight 100

	
	
	{
		map $lightmap
		rgbgen identity
	}


	{
		map textures/sfx/metalfloor_wall_15b.tga
		blendfunc gl_dst_color gl_zero
		rgbgen identity
	}

	{
		map textures/sfx/metalfloor_wall_15bglow.tga
		rgbgen wave sin .1 .05 1 1
		blendfunc gl_one gl_one
	}
	
}


textures/sfx/xblackfog
{
qer_editorimage textures/sfx/blackness.tga
qer_nocarve
surfaceparm	trans
surfaceparm	nonsolid
surfaceparm	fog
surfaceparm	nolightmap
fogparms ( 0 0 0 ) 128

}


textures/sfx/xfinalfog
{
qer_editorimage textures/sfx/xdensegreyfog.tga
qer_nocarve
surfaceparm	trans
surfaceparm	nonsolid
surfaceparm	fog
surfaceparm	nolightmap
fogparms ( 0 0 0 ) 500

}

textures/sfx/xfinalfoginvert
{
qer_editorimage textures/sfx/xdensegreyfog.tga
qer_nocarve
surfaceparm	trans
surfaceparm	nonsolid
surfaceparm	fog
surfaceparm	nolightmap
qer_nocarve
fogparms ( 0 0 0 ) 256

}


textures/sfx/xbluefog
{
	qer_editorimage textures/sfx/xdensegreyfog.tga
	q3map_surfacelight 300
	q3map_lightsubdivide 64
	q3map_lightimage textures/sfx/xbluelightimage.tga	

	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm	nolightmap
	qer_nocarve
	fogparms ( 0.3 0.3 0.9 ) 128

}

textures/sfx/xredfog
{
	qer_editorimage textures/sfx/xdensegreyfog.tga
	q3map_surfacelight 300
	q3map_lightsubdivide 64
	q3map_lightimage textures/sfx/xredlightimage.tga	

	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm	nolightmap
	qer_nocarve
	fogparms ( 0.9 0.3 0.3 ) 128

}

textures/sfx/xbluefogx128
{
	qer_editorimage textures/sfx/xdensegreyfog.tga
	q3map_surfacelight 300
	q3map_lightsubdivide 128

	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm	nolightmap
	qer_nocarve
	fogparms ( 0.3 0.3 0.9 ) 128

}

textures/sfx/beam
{
        surfaceparm trans	
        surfaceparm nomarks	
        surfaceparm nonsolid
	surfaceparm nolightmap
	cull none
	surfaceparm nomipmaps
        //nopicmip
	{
		map textures/sfx/beam.tga
                tcMod Scroll .3 0
                blendFunc add
        }
        // {
	//	map textures/sfx/beam.tga
        //        tcMod Scroll -.3 0
        //        blendFunc GL_ONE GL_ONE
        // }
     
}

textures/sfx/beam_dusty2
{
	qer_editorimage textures/sfx/beam.tga
        surfaceparm trans	
        surfaceparm nomarks	
        surfaceparm nonsolid
	surfaceparm nolightmap
	cull none
	surfaceparm nomipmaps
        //nopicmip
	{
		map textures/sfx/beam_1.tga
           //     tcMod Scroll .3 0
                blendFunc add
        }
 //        {
//		map textures/sfx/beamdust.tga
//		tcmod scale 2 2
//		tcMod turb 0 0.015 0.025 0.05
 //               tcMod Scroll -0.15 0
   //             blendFunc GL_ONE GL_ONE
   //      }
     	//{
	//	map textures/sfx/beam_mask.tga
        //        blendFunc GL_DST_COLOR GL_ONE_MINUS_SRC_COLOR  
      //	}
}


textures/sfx/beam_red
{
        surfaceparm trans	
        surfaceparm nomarks	
        surfaceparm nonsolid
	surfaceparm nolightmap
        qer_trans .5
	cull none
	surfaceparm nomipmaps
        //nopicmip
	{
		map textures/sfx/beam_red.tga
                tcMod Scroll .3 0
                blendFunc add
        }
     
}

textures/sfx/beam_blue
	{
        qer_editorimage textures/sfx/beam_blue4.tga
	//q3map_globaltexture
	surfaceparm trans	
        surfaceparm nomarks	
        surfaceparm nonsolid
	surfaceparm nolightmap
        qer_trans .6
	cull none
	surfaceparm nomipmaps
        //nopicmip
	{
		map textures/sfx/beam_blue4.tga
	//	tcMod scale 0.5 0.5
	//	tcMod turb 0 0.015 0.5 0.07
		tcMod Scroll .3 0
                blendFunc add
        }
     
}

textures/sfx/beam_waterlight1
	{
        qer_editorimage textures/sfx/beam_waterlight.tga
	q3map_globaltexture
	surfaceparm trans	
        surfaceparm nomarks	
        surfaceparm nonsolid
	surfaceparm nolightmap
	q3map_surfacelight 50
        //qer_trans .3
	cull none
	surfaceparm nomipmaps
        //nopicmip
	{
		map textures/sfx/beam_waterlight.tga
		tcMod scale 0.35 0.35
		tcMod turb 0 0.015 0.5 0.07
		tcMod Scroll .15 0
                blendFunc add
        }
     	{
		map textures/sfx/beam_cyan5.tga
		tcMod scale 0.5 0.5
		tcMod turb 0 0.025 0.5 0.03
		tcMod Scroll .15 .15
                blendFunc add
        }

}
textures/sfx/beam_waterlight1
	{
        qer_editorimage textures/sfx/beam_waterlight.tga
	q3map_globaltexture
	surfaceparm trans	
        surfaceparm nomarks	
        surfaceparm nonsolid
	surfaceparm nolightmap
	q3map_surfacelight 50
	surfaceparm nomipmaps
        qer_trans .3
	cull none
        //nopicmip
	{
		map textures/sfx/beam_waterlight.tga
		tcMod scale 0.35 0.35
		tcMod turb 0 0.015 0.5 0.07
		tcMod Scroll .15 0
                blendFunc GL_add
        }
     	{
		map textures/sfx/beam_cyan5.tga
		tcMod scale 0.5 0.5
		tcMod turb 0 0.025 0.5 0.03
		tcMod Scroll .15 .15
                blendFunc add
        }

}
textures/sfx/beam_waterlight2
	{
        qer_editorimage textures/sfx/beam_waterlight.tga
	q3map_globaltexture
	surfaceparm trans	
        surfaceparm nomarks	
        surfaceparm nonsolid
	surfaceparm nolightmap
	q3map_surfacelight 50
        qer_trans 0.6
	cull none
	surfaceparm nomipmaps
        //nopicmip
	{
		map textures/sfx/beam_waterlight.tga
		//tcMod scale 0.35 0.35
		tcMod turb 0 0.015 0.5 0.07
		tcMod Scroll .15 0
                blendFunc add
        }

}
textures/sfx/beam_dusty
	{
        qer_editorimage textures/sfx/beam_3.tga
	surfaceparm trans	
        surfaceparm nomarks	
        surfaceparm nonsolid
	surfaceparm nolightmap
        qer_trans .3
	cull none
	surfaceparm nomipmaps
        //nopicmip
	{
		map textures/sfx/beam_3.tga
	//	tcMod scale 0.35 0.35
		tcMod turb 0 0.015 0.5 0.07
		tcMod Scroll .15 0
                blendFunc add
        }
}

textures/sfx/beam_water
	{
        qer_editorimage textures/sfx/beam_cyan4.tga
	q3map_surfacelight 20
	surfaceparm trans	
        surfaceparm nomarks	
        surfaceparm nonsolid
	surfaceparm nolightmap
        qer_trans .3
	cull none
	surfaceparm nomipmaps
        //nopicmip
	{
		map textures/sfx/beam_cyan4.tga
		tcMod turb 0 0.025 0.5 0.03
		tcMod Scroll .15 0
                blendFunc GL_ONE GL_ONE
        }
     	{
		map textures/sfx/beam_cyan5.tga
		tcMod turb 0 0.025 0.5 0.03
		tcMod Scroll .15 .15
                blendFunc add
        }

}

textures/sfx/beam_water2
	{
        qer_editorimage textures/sfx/beam_cyan5.tga
	q3map_surfacelight 10
	surfaceparm trans	
        surfaceparm nomarks	
        surfaceparm nonsolid
	surfaceparm nolightmap
        qer_trans .3
	cull none
	surfaceparm nomipmaps
        //nopicmip
	{
		map textures/sfx/beam_cyan5.tga
		tcMod turb 0 0.045 0.5 0.03
		tcMod Scroll .15 .015
                blendFunc add
        }  
}

textures/sfx/console01
{
        surfaceparm trans	
        surfaceparm nomarks	
	surfaceparm nolightmap
        qer_trans .5
        q3map_surfacelight 150
        q3map_lightimage textures/sfx/console01.tga
	cull none
        //nopicmip
	{
		clampmap textures/sfx/console01.tga
               
		tcMod rotate 20
		blendFunc GL_ONE GL_ONE
	}
        {
		clampmap textures/sfx/console02.tga
                
		tcMod rotate -200
		blendFunc GL_ONE GL_ONE
	}
       
}
textures/sfx/console03
{
        surfaceparm trans	
        surfaceparm nomarks	
	surfaceparm nolightmap
        q3map_surfacelight 150
        q3map_lightimage textures/sfx/console03.tga
	cull none
        //nopicmip
	{
		clampmap textures/sfx/console01.tga
               
		tcMod rotate 20
		blendFunc GL_ONE GL_ONE
	}
        {
		clampmap textures/sfx/console02.tga
                
		tcMod rotate -400
		blendFunc GL_ONE GL_ONE
	}
          {
		clampmap textures/sfx/console03.tga
                
		tcMod rotate 40
		blendFunc GL_ONE GL_ONE
	}
}
textures/sfx/glass
{
	qer_editorimage textures/base_wall/oldwindow.tga
	surfaceparm	trans
	cull none			

	{
		map $lightmap
		tcgen environment
		tcmod scale .25 .25
		blendfunc gl_one gl_one
	}

	{
		map textures/base_wall/oldwindow.tga
		blendFunc GL_ONE_MINUS_SRC_ALPHA GL_SRC_ALPHA
		rgbGen identity
	}

	{
		map $lightmap
		blendfunc gl_dst_color gl_zero
	}		
}


textures/sfx/fan
{
        surfaceparm trans	
        surfaceparm nomarks	
	cull none
        nopicmip
	{
		clampmap textures/sfx/fan.tga
		tcMod rotate 256 
		blendFunc GL_ONE GL_ZERO
		alphaFunc GE128
		depthWrite
		rgbGen identity
	}
	{
		map $lightmap
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
		depthFunc equal
	}
}

textures/sfx/fan2
{
        surfaceparm trans	
        surfaceparm nomarks	
	cull disable
        nopicmip
	{
		clampmap textures/sfx/fan2.tga
		tcMod rotate 256 
		//blendFunc GL_ONE GL_ZERO
		//alphaFunc GE128
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
                alphaFunc GT0
		depthWrite
		rgbGen identity
	}
	{
		map $lightmap
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
		depthFunc equal
	}
}
textures/sfx/fan_grate
{
        surfaceparm trans	
        surfaceparm nomarks	
        surfaceparm	metalsteps	
	cull none
        nopicmip
	{
		map textures/sfx/fan_grate.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc GE128
		depthWrite
		rgbGen identity
	}
	{
		map $lightmap
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
		depthFunc equal
	}
}
textures/sfx/fan_grate2
{
        surfaceparm trans	
        surfaceparm nomarks	
        surfaceparm	metalsteps	
	cull none
        nopicmip
	{
		map textures/sfx/fan_grate2.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc GE128
		depthWrite
		rgbGen identity
	}
	{
		map $lightmap
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
		depthFunc equal
	}
}
textures/sfx/fanfx
// fan shadow
{
	qer_editorimage textures/sfx/fanfx.tga
        surfaceparm nolightmap
        surfaceparm trans	
        surfaceparm nomarks	
	cull none
        nopicmip

	{
		clampmap textures/sfx/fanfx.tga
		tcMod rotate 256 
		blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen exactVertex
                depthWrite
		//rgbGen identity
	}
}

textures/sfx/metalfloor_wall_5_glow
{
	qer_editorimage textures/sfx/metalfloor_wall_5_glow.tga
	q3map_lightimage textures/sfx/metalfloor_wall_5bglowblu.tga
//	q3map_surfacelight 100
//	q3map_lightsubdivide 32
	{
		map $lightmap
		rgbgen identity
	}

	{
		map textures/base_wall/metalfloor_wall_5.tga
		blendfunc gl_dst_color gl_zero
		rgbgen identity
	}

	{
		map textures/sfx/metalfloor_wall_5_glow.tga
		blendfunc gl_one gl_one
		rgbgen wave sin .1 .4 1 0.3
	}

}


textures/sfx/metaltech12darkfinal_blueglow
{
	qer_editorimage textures/sfx/metaltech01blueglow.tga
	q3map_lightimage textures/sfx/metaltech01blueglow.tga
	q3map_surfacelight 100
	{
		map $lightmap
		rgbgen identity
	}

	{
		map textures/base_wall/metaltech12darkfinal.tga
		blendfunc gl_dst_color gl_zero
		rgbgen identity
	}

	{
		map textures/sfx/metaltech01blueglow.tga
		blendfunc gl_one gl_one
		rgbgen wave sin .1 .1 1 0.1
	}

}


textures/sfx/zap_scroll1000
{
        q3map_surfacelight	1000
        surfaceparm	trans
	surfaceparm nomarks
	surfaceparm nolightmap
	qer_editorimage textures/sfx/zap_scroll.tga
	cull none
	
	{
		map textures/sfx/zap_scroll.tga
		blendFunc GL_ONE GL_ONE
                rgbgen wave triangle .8 2 0 7
                tcMod scroll 0 1
	}	
        {
		map textures/sfx/zap_scroll.tga
		blendFunc GL_ONE GL_ONE
                rgbgen wave triangle 1 1.4 0 5
                tcMod scale  -1 1
                tcMod scroll 0 1
	}	
        {
		map textures/sfx/zap_scroll2.tga
		blendFunc GL_ONE GL_ONE
                rgbgen wave triangle 1 1.4 0 6.3
                tcMod scale  -1 1
                tcMod scroll 2 1
	}	
        {
		map textures/sfx/zap_scroll2.tga
		blendFunc GL_ONE GL_ONE
                rgbgen wave triangle 1 1.4 0 7.7
                tcMod scroll -1.3 1
	}	
}


textures/gothic_block/killblock_i4b
{
        q3map_surfacelight 300
	q3map_lightimage textures/gothic_block/killblock_i4glow.tga
	qer_editorimage textures/gothic_block/killblock_i4.tga

	
	{
		map textures/gothic_block/killblock_i4.tga
		rgbgen identity		
	}	

	{
		map $lightmap
		rgbGen identity
		blendfunc gl_dst_color gl_zero
	}
	{
		map  textures/gothic_block/killblock_i4glow.tga
		blendFunc GL_ONE GL_ONE
                rgbgen wave sin .9 .1 0 .1
	}
	
}


textures/sfx/donna
{
	deformVertexes autoSprite
        q3map_surfacelight	300

	surfaceparm	trans
	surfaceparm nomarks
	surfaceparm nolightmap
	cull none
	
	{
		Map textures/sfx/donna.tga
		blendFunc GL_ONE GL_ONE
                rgbgen wave triangle 1 2 0 7
	}	
{
		Map textures/sfx/donna.tga
		blendFunc GL_ONE GL_ONE
                rgbgen wave sin 1 2 0 8
	}
	
}

textures/sfx/xian_dm3padwall
	{
	q3map_surfacelight 100
	q3map_lightimage textures/sfx/xian_dm3padwallglow.tga
	qer_editorimage textures/sfx/xian_dm3padwall.tga
	
	{
		map textures/sfx/xian_dm3padwall.tga
		rgbGen identity
	}
	
	{
		map $lightmap
		rgbGen identity
		blendfunc gl_dst_color gl_zero
	}


	{
		map textures/sfx/xian_dm3padwallglow.tga
		blendfunc gl_one gl_one
		rgbgen wave sin 0 1 0 .5
		tcmod scale 1 .05
		tcmod scroll 0 1
	}
	
}

textures/sfx/xmetalfloor_wall_5b
{
	qer_editorimage textures/sfx/metalfloor_wall_5b.tga

	{
		map textures/sfx/metalfloor_wall_7b.tga
		rgbGen identity
	}
	{
		map $lightmap
		rgbGen identity
		blendfunc gl_dst_color gl_zero
	}
	{
		map textures/sfx/metalfloor_wall_5bglowblu.tga
		blendfunc gl_one gl_one
		rgbgen wave sin .5 .2 0 .1	
	}
}

textures/sfx/border11c
{
	q3map_surfacelight 300
	q3map_lightimage textures/base_trim/border11c_pulse1.tga
	qer_editorimage textures/base_trim/border11c.tga
	
	{
		map textures/base_trim/border11c.tga
		rgbGen identity
	}
	
	{
		map $lightmap
		rgbGen identity
		blendfunc gl_dst_color gl_zero
	}

	{
		map textures/base_trim/border11c_light.tga
		blendfunc gl_one gl_one
		rgbgen wave sin 1 .1 0 5
	}


	{
		map textures/base_trim/border11c_pulse1b.tga
		blendfunc gl_one gl_one
//		tcmod scale .01 1
		tcmod scale .035 1
//		tcmod scroll -0.5 0
		tcmod scroll -0.65 0


	}

	
}



textures/sfx/zap_scroll
{
        q3map_surfacelight	300
        surfaceparm	trans
	surfaceparm nomarks
	surfaceparm nolightmap
	cull none
	
	{
		Map textures/sfx/zap_scroll.tga
		blendFunc GL_ONE GL_ONE
                rgbgen wave triangle .8 2 0 7
                tcMod scroll 0 1
	}	
        {
		Map textures/sfx/zap_scroll.tga
		blendFunc GL_ONE GL_ONE
                rgbgen wave triangle 1 1.4 0 5
                tcMod scale  -1 1
                tcMod scroll 0 1
	}	
        {
		Map textures/sfx/zap_scroll2.tga
		blendFunc GL_ONE GL_ONE
                rgbgen wave triangle 1 1.4 0 6.3
                tcMod scale  -1 1
                tcMod scroll 2 1
	}	
        {
		Map textures/sfx/zap_scroll2.tga
		blendFunc GL_ONE GL_ONE
                rgbgen wave triangle 1 1.4 0 7.7
                tcMod scroll -1.3 1
	}	
}
textures/sfx/x_conduit
{
	q3map_lightimage textures/sfx/x_conduit.tga
	surfaceparm nomarks
	q3map_surfacelight 100
	light 1
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/sfx/x_conduit.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}

	{	animMap 10 textures/sfx/x_conduit2.tga textures/sfx/x_conduit3.tga 
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
	}

	//{	
	//	map textures/sfx/x_conduit2.tga
	//	blendfunc GL_ONE GL_ONE
       //         rgbGen wave sin .5 0.5 0 5
	//}
        {	
		map textures/sfx/x_conduit2.tga
		blendfunc GL_ONE GL_ONE
                 tcmod scale -1 1
                rgbGen wave sin .5 0.5 0 7
	}
        {	
		map textures/sfx/x_conduit3.tga
		blendfunc GL_ONE GL_ONE
                tcmod scale -1 1
                 rgbgen wave triangle .2 1 0 9
	}
        //{	
	//	map textures/sfx/x_conduit3.tga
	//	blendfunc GL_ONE GL_ONE
       //         rgbGen wave sin .5 1 0 3
	//}
}
textures/sfx/swirl_r1
{
        deformVertexes wave 100 sin 1 2 .1 1
        q3map_surfacelight	300

	surfaceparm	trans
	surfaceparm nomarks
        surfaceparm nonsolid
	surfaceparm nolightmap
	cull none

               
	
	{
		clampmap textures/sfx/swirl_r1.tga
		blendFunc GL_ONE GL_ONE
                tcMod rotate -188
	}	
        {
		clampmap textures/sfx/swirl_r2.tga
		blendFunc GL_ONE GL_ONE
                tcMod rotate 333
	}
       // {
	//	clampmap textures/sfx/swirl_r2.tga
	//	blendFunc GL_ONE GL_ONE
      //          tcMod rotate 20
	//}
	
}
textures/sfx/swirl_b1
{
        deformVertexes wave 100 sin 1 2 .1 1
        q3map_surfacelight	300

	surfaceparm	trans
	surfaceparm nomarks
	surfaceparm nolightmap
        surfaceparm nonsolid
	cull none
	
	{
		clampmap textures/sfx/swirl_b1.tga
		blendFunc GL_ONE GL_ONE
                tcMod rotate -188
	}	
        {
		clampmap textures/sfx/swirl_b2.tga
		blendFunc GL_ONE GL_ONE
                tcMod rotate 333
	}
        //{
	//	clampmap textures/sfx/swirl_b2.tga
	//	blendFunc GL_ONE GL_ONE
        //        tcMod rotate 20
	//}
	
}
textures/sfx/steam_01
{

	surfaceparm	trans
	surfaceparm nomarks
	surfaceparm nolightmap
	cull disable
       
	{
		Map textures/sfx/steam_01.tga
		blendFunc GL_ONE GL_ONE
                tcMod scale 1 .4
	        tcMod scroll 0 .3
	}	
{
                Map textures/sfx/steam_01.tga
		blendFunc GL_ONE GL_ONE
                tcMod scale -1 .2
	        tcMod scroll 0 .3
	}
	
}
textures/sfx/zap_ball
{
	deformVertexes autoSprite
        q3map_surfacelight	300

	surfaceparm	trans
	surfaceparm nomarks
	surfaceparm nolightmap
	cull none
	
	{
		clampmap textures/sfx/zap_ball.tga
		blendFunc GL_ONE GL_ONE
                rgbgen wave triangle 1 2 0 7
                tcMod rotate 103
	}	
{
		clampmap textures/sfx/zap_ball2.tga
		blendFunc GL_ONE GL_ONE
                rgbgen wave sin 1 2 0 8
                tcMod rotate -180
	}
	
}
textures/sfx/jacobs_x
{
        deformVertexes wave 100 sin 0 1 0 5
        q3map_surfacelight	300

	surfaceparm	trans
	surfaceparm nomarks
	surfaceparm nolightmap
	cull none
	
	{
		Map textures/sfx/jacobs_x.tga
		blendFunc GL_ONE GL_ONE
                rgbgen wave triangle 1 2 0 7
                //tcMod rotate 103
	}	
}
textures/sfx/xlargeblockfloor3_pent
{
	q3map_lightimage textures/sfx/pentagramfloor_blue3glowtest.tga
	q3map_surfacelight 100
	surfaceparm nomarks
	
	{
		map $lightmap
		rgbgen identity
	}

	{
		map textures/sfx/xlargeblockfloor3_pent.tga
		blendfunc gl_dst_color gl_zero
		rgbgen identity
	}

	{
		map textures/sfx/pentagramfloor_blue3glowtest.tga
		blendfunc gl_one gl_one
		rgbgen wave sin .9 .1 0 5
	}

}

textures/sfx/xclang_floor2_bouncy
{

	//q3map_surfacelight 2000
	surfaceparm nodamage
	q3map_lightimage textures/sfx/jumppadsmall.tga	
	q3map_surfacelight 400

	
	{
		map textures/sfx/xclang_floor2_bouncy.tga
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
		clampmap textures/sfx/jumppadsmall.tga
		blendfunc gl_one gl_one
		tcMod stretch sin 1.2 .8 0 1.5
		rgbGen wave square .5 .5 .25 1.5
	}

	//	END
}

textures/sfx/clangdark_bounce
{

	//q3map_surfacelight 2000
	surfaceparm nodamage
	q3map_lightimage textures/sfx/jumppadsmall.tga	
	q3map_surfacelight 400

	
	{
		map textures/sfx/clangdark_bounce.tga
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
		clampmap textures/sfx/jumppadsmall.tga
		blendfunc gl_one gl_one
		tcMod stretch sin 1.2 .8 0 1.5
		rgbGen wave square .5 .5 .25 1.5
	}

	//	END
}


textures/sfx/metalbridge06_bounce
{

	//q3map_surfacelight 2000
	surfaceparm nodamage
	q3map_lightimage textures/sfx/jumppadsmall.tga	
	q3map_surfacelight 400

	
	{
		map textures/sfx/metalbridge06_bounce.tga
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
		clampmap textures/sfx/jumppadsmall.tga
		blendfunc gl_one gl_one
		tcMod stretch sin 1.2 .8 0 1.5
		rgbGen wave square .5 .5 .25 1.5
	}

	//	END
}

textures/sfx/metaltechfloor02final_bounce
{

	//q3map_surfacelight 2000
	surfaceparm nodamage
	q3map_lightimage textures/sfx/metaltechfloor02final_bounce.tga	
	q3map_surfacelight 400

	
	{
		map textures/sfx/metaltechfloor02final_bounce.tga
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
		clampmap textures/sfx/jumppadsmall.tga
		blendfunc gl_one gl_one
		tcMod stretch sin 1.2 .8 0 1.5
		rgbGen wave square .5 .5 .25 1.5
	}

	//	END
}


textures/sfx/metaltech12final_bounce
{

	//q3map_surfacelight 2000
	surfaceparm nodamage
//	q3map_lightimage textures/sfx/metaltech12final_bounce.tga	
	q3map_lightimage textures/sfx/jumppadsmall.tga	
	q3map_surfacelight 400

	
	{
		map textures/sfx/metaltech12final_bounce.tga
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
		clampmap textures/sfx/jumppadsmall.tga
		blendfunc gl_one gl_one
		tcMod stretch sin 1.2 .8 0 1.5
		rgbGen wave square .5 .5 .25 1.5
	}

	//	END
}


textures/sfx/metaltech12darkfinal_bounce
{

	//q3map_surfacelight 2000
	surfaceparm nodamage
//	q3map_lightimage textures/sfx/metaltech12darkfinal_bounce.tga	
	q3map_lightimage textures/sfx/jumppadsmall.tga	
	q3map_surfacelight 400

	
	{
		map textures/sfx/metaltech12darkfinal_bounce.tga
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
		clampmap textures/sfx/jumppadsmall.tga
		blendfunc gl_one gl_one
		tcMod stretch sin 1.2 .8 0 1.5
		rgbGen wave square .5 .5 .25 1.5
	}

	//	END
}


textures/sfx/metaltechfloor01final_bounce
{

	//q3map_surfacelight 2000
	surfaceparm nodamage
	q3map_lightimage textures/sfx/metaltechfloor01final_bounce.tga	
	q3map_surfacelight 400

	
	{
		map textures/sfx/metaltechfloor01final_bounce.tga
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
		clampmap textures/sfx/jumppadsmall.tga
		blendfunc gl_one gl_one
		tcMod stretch sin 1.2 .8 0 1.5
		rgbGen wave square .5 .5 .25 1.5
	}

	//	END
}



textures/sfx/demonltblackfinal
{
	q3map_lightimage textures/sfx/demonltblackfinal_glow2.tga
	q3map_surfacelight 100
	surfaceparm nomarks
	
	{
		map $lightmap
		rgbgen identity
	}

	{
		map textures/sfx/demonltblackfinal.tga
		blendfunc gl_dst_color gl_zero
		rgbgen identity
	}

	{
		map textures/sfx/demonltblackfinal_glow2.tga
		blendfunc gl_one gl_one
		rgbgen wave sin .9 .1 0 5
	}

}
textures/base_wall/c_met5_2_trans
{
	qer_editorimage textures/base_wall/c_met5_2.tga
	surfaceparm nonsolid
	{
		map $lightmap
		rgbGen identity
	
	}
	{
		map textures/base_wall/c_met5_2.tga
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO

	
	}
}



textures/sfx/metalfloor_glass
{
	qer_editorimage textures/sfx/metalfloor_wall_15glass.tga
	qer_trans	0.4
	surfaceparm trans
	cull disable
//	rgbGen vertex
//	surfaceparm nolightmap
	{
		map textures/sfx/metalfloor_wall_15glass.tga
		rgbGen identity
		rgbGen vertex
		blendFunc GL_ONE_MINUS_DST_COLOR GL_ZERO
	}
	{
		map textures/sfx/metalfloor_wall_15glass.tga
		alphaFunc GT0
		rgbGen identity
		blendfunc GL_DST_COLOR GL_ONE
	}
}

textures/sfx/xmetalfloor_wall_14b
{
	qer_editorimage textures/sfx/metalfloor_wall_14b.tga
	q3map_lightimage textures/sfx/metalfloor_wall_14bglow2.tga
	q3map_surfacelight 50
	{
		map textures/sfx/metalfloor_wall_14b.tga
		rgbGen identity
	}
	{
		map $lightmap
		rgbGen identity
		blendfunc gl_dst_color gl_zero
	}
	{
		map textures/sfx/metalfloor_wall_14bglow2.tga
		blendfunc gl_one gl_one
	}
}

textures/sfx/xmetalfloor_wall_9b
{
	qer_editorimage textures/sfx/metalfloor_wall_9b.tga

	{
		map textures/sfx/metalfloor_wall_9b.tga
		rgbGen identity
	}
	{
		map $lightmap
		rgbGen identity
		blendfunc gl_dst_color gl_zero
	}
	{
		map textures/sfx/metalfloor_wall_9bglow.tga
		blendfunc gl_one gl_one
	}
}


models/mapobjects/portal_2/portal_3
{
	//	*************************************************
	//	*      	Portal Sides				*
	//	*      	April 30 1999				*	
	//	*	Please Comment Changes			*
	//	*************************************************


	{
		map models/mapobjects/portal_2/portal_3.tga
		rgbGen vertex
	}

	{
		map models/mapobjects/portal_2/portal_3_glo.tga
		blendfunc gl_one gl_one
		rgbgen wave inversesawtooth 0 1 .2 .5
	}

}


textures/sfx/portal_sfx_ring
{

	//	*************************************************
	//	*      	Portal Ring				*
	//	*      	April 30 1999				*	
	//	*	Please Comment Changes			*
	//	*************************************************
		
		deformVertexes wave 100 sin 0 2 0 .5
		cull none
	
	{
		map textures/sfx/portal_sfx_ring_blue1.tga 
		blendfunc gl_src_alpha gl_one_minus_src_alpha
	}

	
	{	
		map textures/sfx/portal_sfx_ring_electric.tga 
		blendfunc gl_one gl_one
		rgbgen wave inversesawtooth 0 1 .2 .5
		tcmod scroll 0 .5

	}

	{
		map textures/sfx/portal_sfx1.tga
		blendfunc gl_dst_color gl_zero
		tcMod rotate 360
	}

	{
		map textures/sfx/portal_sfx_ring.tga
		blendfunc gl_one gl_one
		rgbgen wave inversesawtooth 0 .5 .2 .5
	}

	

	

}

textures/sfx/small_smoke1_1
{

	//	*************************************************
	//	*      	Smoke					*
	//	*      	June 18 1999				*	
	//	*	Please Comment Changes			*
	//	*************************************************
	
	deformVertexes autoSprite
	surfaceparm nomarks
	surfaceparm nolightmap
	cull none
	

	{
		animMap 10 textures/sfx/small_smoke1_1.tga textures/sfx/small_smoke1_2.tga textures/sfx/small_smoke1_3.tga textures/sfx/small_smoke1_4.tga textures/sfx/small_smoke1_5.tga textures/sfx/small_smoke1_6.tga 
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10	
	}
	
	{
		animMap 10 textures/sfx/small_smoke1_2.tga textures/sfx/small_smoke1_3.tga textures/sfx/small_smoke1_4.tga textures/sfx/small_smoke1_5.tga textures/sfx/small_smoke1_6.tga textures/sfx/small_smoke1_1.tga 
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}	

}



textures/sfx/xflame2
{

	//	*************************************************
	//	*      	Yellow Flame Surface Light 5500		*
	//	*      	April 30 1999				*	
	//	*	Please Comment Changes			*
	//	*************************************************
	
	surfaceparm nomarks
	surfaceparm nolightmap
	cull none
	q3map_surfacelight 5500
	qer_editorimage textures/sfx/flame1.tga
	

	{
		animMap 10 textures/sfx/flame1.tga textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
		
	}	
	{
		animMap 10 textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga textures/sfx/flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}	


	{
		map textures/sfx/flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6	
	}

}

textures/sfx/xflame2_2250
{
	surfaceparm nomarks
	surfaceparm nolightmap
	cull none
	q3map_surfacelight 2250
	qer_editorimage textures/sfx/flame1.tga
	

	{
		animMap 10 textures/sfx/flame1.tga textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
		
	}	
	{
		animMap 10 textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga textures/sfx/flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}	


	{
		map textures/sfx/flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6	
	}

}

textures/sfx/xflame2_1800
{
	surfaceparm nomarks
	surfaceparm nolightmap
	cull none
	q3map_surfacelight 1800
	qer_editorimage textures/sfx/flame1.tga
	

	{
		animMap 10 textures/sfx/flame1.tga textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
		
	}	
	{
		animMap 10 textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga textures/sfx/flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}	


	{
		map textures/sfx/flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6	
	}

}


// Tim's goofy experiment
textures/sfx/grid

{

	surfaceparm trans
	qer_editorimage	textures/sfx/grid.tga

	
	

	{
		map textures/sfx/grid.tga
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}

}


textures/sfx/launchpad_diamond
{
	//	*************************************************
	//	*      	Launchpad on Diamond			*
	//	*      	April 30 1999				*	
	//	*	Please Comment Changes			*
	//	*************************************************
	
	{
		map $lightmap
		rgbGen identity
	}

	{ 
		map textures/sfx/launchpad_diamond.tga
		rgbGen identity
		blendfunc gl_dst_color gl_zero
	}

	{	
		map textures/sfx/launchpad_dot.tga
		blendfunc gl_one gl_one	
		rgbgen wave inversesawtooth 0 1 0 1	
	}

	// 	CHANGED BECAUSE OF RAGE PRO PROBLEM
	//	{	
	//		map textures/sfx/launchpad_arrow.tga		
	//		blendfunc gl_src_alpha gl_one	
	//		tcmod scroll 0 2
	//		rgbgen wave square 0 1 0 2
	//		alphagen wave square 0 1 .1 2
	//	}

	// 	TEMPORARY FIX TO GET AROUND ALPHA BLEND IN RAGE PRO
	{ 
		animmap 4 textures/sfx/launchpad_arrow.tga textures/sfx/launchpad_arrow2.tga textures/sfx/launchpad_arrow2.tga textures/sfx/launchpad_arrow2.tga
		blendfunc gl_one gl_one
		tcmod scroll 0 2
	}

}
textures/sfx/launchpad_blocks18d
{
	//	*************************************************
	//	*      	Launchpad on Blocks18d			*
	//	*      	October 09 1999				*	
	//	*	Please Comment Changes			*
	//	*************************************************
	
	{
		map $lightmap
		rgbGen identity
	}

	{ 
		map textures/sfx/launchpad_blocks18d.tga
		rgbGen identity
		blendfunc gl_dst_color gl_zero
	}

	{	
		map textures/sfx/launchpad_dot.tga
		blendfunc gl_one gl_one	
		rgbgen wave inversesawtooth 0 1 0 1	
	}

	// 	CHANGED BECAUSE OF RAGE PRO PROBLEM
	//	{	
	//		map textures/sfx/launchpad_arrow.tga		
	//		blendfunc gl_src_alpha gl_one	
	//		tcmod scroll 0 2
	//		rgbgen wave square 0 1 0 2
	//		alphagen wave square 0 1 .1 2
	//	}

	// 	TEMPORARY FIX TO GET AROUND ALPHA BLEND IN RAGE PRO
	{ 
		animmap 4 textures/sfx/launchpad_arrow.tga textures/sfx/launchpad_arrow2.tga textures/sfx/launchpad_arrow2.tga textures/sfx/launchpad_arrow2.tga
		blendfunc gl_one gl_one
		tcmod scroll 0 2
	}

}
textures/sfx/launchpad_blocks17
{
	//	*************************************************
	//	*      	Launchpad on Blocks17			*
	//	*      	November 08 1999			*	
	//	*	Please Comment Changes			*
	//	*************************************************
	
	{
		map $lightmap
		rgbGen identity
	}

	{ 
		map textures/sfx/launchpad_blocks17.tga
		rgbGen identity
		blendfunc gl_dst_color gl_zero
	}

	{	
		map textures/sfx/launchpad_dot.tga
		blendfunc gl_one gl_one	
		rgbgen wave inversesawtooth 0 1 0 1	
	}

	// 	CHANGED BECAUSE OF RAGE PRO PROBLEM
	//	{	
	//		map textures/sfx/launchpad_arrow.tga		
	//		blendfunc gl_src_alpha gl_one	
	//		tcmod scroll 0 2
	//		rgbgen wave square 0 1 0 2
	//		alphagen wave square 0 1 .1 2
	//	}

	// 	TEMPORARY FIX TO GET AROUND ALPHA BLEND IN RAGE PRO
	{ 
		animmap 4 textures/sfx/launchpad_arrow.tga textures/sfx/launchpad_arrow2.tga textures/sfx/launchpad_arrow2.tga textures/sfx/launchpad_arrow2.tga
		blendfunc gl_one gl_one
		tcmod scroll 0 2
	}

}
textures/sfx/launchpad_metalbridge04d
{
	//	*************************************************
	//	*      	Launchpad on metalbridge04d	        *
	//	*      	June 15 1999				*	
	//	*	Please Comment Changes			*
	//	*************************************************
	
	{
		map $lightmap
		rgbGen identity
	}

	{ 
		map textures/sfx/launchpad_metalbridge04d.tga
		rgbGen identity
		blendfunc gl_dst_color gl_zero
	}

	{	
		map textures/sfx/launchpad_dot.tga
		blendfunc gl_one gl_one	
		rgbgen wave inversesawtooth 0 1 0 1	
	}

	// 	CHANGED BECAUSE OF RAGE PRO PROBLEM
	//	{	
	//		map textures/sfx/launchpad_arrow.tga		
	//		blendfunc gl_src_alpha gl_one	
	//		tcmod scroll 0 2
	//		rgbgen wave square 0 1 0 2
	//		alphagen wave square 0 1 .1 2
	//	}

	// 	TEMPORARY FIX TO GET AROUND ALPHA BLEND IN RAGE PRO
	{ 
		animmap 4 textures/sfx/launchpad_arrow.tga textures/sfx/launchpad_arrow2.tga textures/sfx/launchpad_arrow2.tga textures/sfx/launchpad_arrow2.tga
		blendfunc gl_one gl_one
		tcmod scroll 0 2
	}

}

textures/sfx/flame1km
{

	//	*************************************************
	//	*      	Yellow Flame 				*
	//	*      	May 20 1999 Surface Light 4000	*	
	//	*	Please Comment Changes			*
	//	*************************************************
	
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm trans
	cull none
	q3map_surfacelight 4000
	

	{
		animMap 10 textures/sfx/flame1.tga textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
		
	}	
	{
		animMap 10 textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga textures/sfx/flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}	


	{
		map textures/sfx/flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6	
	}

}
textures/sfx/flame1
{

	//	*************************************************
	//	*      	Yellow Flame 				*
	//	*      	April 30 1999 Surface Light 7500	*	
	//	*	Please Comment Changes			*
	//	*************************************************
	
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm trans
	cull none
	q3map_surfacelight 7500
	

	{
		animMap 10 textures/sfx/flame1.tga textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
		
	}	
	{
		animMap 10 textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga textures/sfx/flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}	


	{
		map textures/sfx/flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6	
	}

}
textures/sfx/flameanim_dimmer
{

	//	*************************************************
	//	*      	Yellow Flame 				*
	//	*      	May 2, 199 Surface Light 2500    	*	
	//	*	Please Comment Changes			*
	//	*************************************************
	qer_editorimage textures/sfx/flame6.tga
	q3map_lightimage textures/sfx/flame6.tga
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm nolightmap
	cull none
	q3map_surfacelight 1500
	// light changed to lower value than flame1 .... PAJ
	// added a lightimage function to take color from a frame

	{
		animMap 10 textures/sfx/flame1.tga textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
		
	}	
	{
		animMap 10 textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga textures/sfx/flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}	


	{
		map textures/sfx/flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6	
	}

}
textures/sfx/flameanim_blue
{

	//	*************************************************
	//	*      	Blue Flame 				*
	//	*      	July 20, 1999 Surface Light 1800    	*	
	//	*	Please Comment Changes			*
	//	*************************************************
	qer_editorimage textures/sfx/b_flame7.tga
	q3map_lightimage textures/sfx/b_flame7.tga
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm nolightmap
	cull none
	q3map_surfacelight 1800
	// texture changed to blue flame.... PAJ
	// 9/01 light dropped from 2500 to 1800 .... PAJ

	{
		animMap 10 textures/sfx/b_flame1.tga textures/sfx/b_flame2.tga textures/sfx/b_flame3.tga textures/sfx/b_flame4.tga textures/sfx/b_flame5.tga textures/sfx/b_flame6.tga textures/sfx/b_flame7.tga textures/sfx/b_flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
		
	}	
	{
		animMap 10 textures/sfx/b_flame2.tga textures/sfx/b_flame3.tga textures/sfx/b_flame4.tga textures/sfx/b_flame5.tga textures/sfx/b_flame6.tga textures/sfx/b_flame7.tga textures/sfx/b_flame8.tga textures/sfx/b_flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}	


	{
		map textures/sfx/b_flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6	
	}

}
textures/sfx/flameanim_blue_pj
{

	//	*************************************************
	//	*      	Blue Flame 				*
	//	*      	July 20, 1999 Surface Light 2500    	*	
	//	*	Please Comment Changes			*
	//	*************************************************
	qer_editorimage textures/sfx/b_flame7.tga
	q3map_lightimage textures/sfx/b_flame7.tga
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm nolightmap
	cull none
	q3map_surfacelight 2500
	// texture changed to blue flame.... PAJ
	

	{
		animMap 10 textures/sfx/b_flame1.tga textures/sfx/b_flame2.tga textures/sfx/b_flame3.tga textures/sfx/b_flame4.tga textures/sfx/b_flame5.tga textures/sfx/b_flame6.tga textures/sfx/b_flame7.tga textures/sfx/b_flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
		
	}	
	{
		animMap 10 textures/sfx/b_flame2.tga textures/sfx/b_flame3.tga textures/sfx/b_flame4.tga textures/sfx/b_flame5.tga textures/sfx/b_flame6.tga textures/sfx/b_flame7.tga textures/sfx/b_flame8.tga textures/sfx/b_flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}	


	{
		map textures/sfx/b_flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6	
	}

}

textures/sfx/flameanim_blue_nolight
{

	//	*************************************************
	//	*      	Blue Flame 				*
	//	*      	aug 18, 1999 Surface Light 2500    	*	
	//	*	Please Comment Changes			*
	//	*************************************************
	qer_editorimage textures/sfx/b_flame7.tga
	q3map_lightimage textures/sfx/b_flame7.tga
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm nolightmap
	cull none
	//q3map_surfacelight 2500
	// texture changed to blue flame.... PAJ

	{
		animMap 10 textures/sfx/b_flame1.tga textures/sfx/b_flame2.tga textures/sfx/b_flame3.tga textures/sfx/b_flame4.tga textures/sfx/b_flame5.tga textures/sfx/b_flame6.tga textures/sfx/b_flame7.tga textures/sfx/b_flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
		
	}	
	{
		animMap 10 textures/sfx/b_flame2.tga textures/sfx/b_flame3.tga textures/sfx/b_flame4.tga textures/sfx/b_flame5.tga textures/sfx/b_flame6.tga textures/sfx/b_flame7.tga textures/sfx/b_flame8.tga textures/sfx/b_flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}	


	{
		map textures/sfx/b_flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6	
	}

}

textures/sfx/flameanim_red
{

	//	*************************************************
	//	*      	Red Flame 				*
	//	*      	July 20, 1999 Surface Light  1800    	*	
	//	*	Please Comment Changes			*
	//	*************************************************
	qer_editorimage textures/sfx/r_flame3.tga
	q3map_lightimage textures/sfx/r_flame3.tga
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm nolightmap
	cull none
	q3map_surfacelight 1800
	// texture changed to red flame.... PAJ
	// 9/01 light dropped from 2500 to 1800

	{
		animMap 10 textures/sfx/r_flame1.tga textures/sfx/r_flame2.tga textures/sfx/r_flame3.tga textures/sfx/r_flame4.tga textures/sfx/r_flame5.tga textures/sfx/r_flame6.tga textures/sfx/r_flame7.tga textures/sfx/r_flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
		
	}	
	{
		animMap 10 textures/sfx/r_flame2.tga textures/sfx/r_flame3.tga textures/sfx/r_flame4.tga textures/sfx/r_flame5.tga textures/sfx/r_flame6.tga textures/sfx/r_flame7.tga textures/sfx/r_flame8.tga textures/sfx/r_flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}	


	{
		map textures/sfx/r_flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6	
	}

}

textures/sfx/flameanim_red_pj
{

	//	*************************************************
	//	*      	Red Flame 				*
	//	*      	July 20, 1999 Surface Light 2500    	*	
	//	*	Please Comment Changes			*
	//	*************************************************
	qer_editorimage textures/sfx/r_flame3.tga
	q3map_lightimage textures/sfx/r_flame3.tga
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm nolightmap
	cull none
	q3map_surfacelight 2500
	// texture changed to red flame.... PAJ

	{
		animMap 10 textures/sfx/r_flame1.tga textures/sfx/r_flame2.tga textures/sfx/r_flame3.tga textures/sfx/r_flame4.tga textures/sfx/r_flame5.tga textures/sfx/r_flame6.tga textures/sfx/r_flame7.tga textures/sfx/r_flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
		
	}	
	{
		animMap 10 textures/sfx/r_flame2.tga textures/sfx/r_flame3.tga textures/sfx/r_flame4.tga textures/sfx/r_flame5.tga textures/sfx/r_flame6.tga textures/sfx/r_flame7.tga textures/sfx/r_flame8.tga textures/sfx/r_flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}	


	{
		map textures/sfx/r_flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6	
	}

}

textures/sfx/flameanim_red_nolight
{

	//	*************************************************
	//	*      	Red Flame (no light)			*
	//	*      	Sept. 5, 1999 No Surface Light	    	*	
	//	*	Please Comment Changes			*
	//	*************************************************
	qer_editorimage textures/sfx/r_flame3.tga
	q3map_lightimage textures/sfx/r_flame3.tga
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm nolightmap
	cull none
	// texture changed to red flame.... PAJ
	

	{
		animMap 10 textures/sfx/r_flame1.tga textures/sfx/r_flame2.tga textures/sfx/r_flame3.tga textures/sfx/r_flame4.tga textures/sfx/r_flame5.tga textures/sfx/r_flame6.tga textures/sfx/r_flame7.tga textures/sfx/r_flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
		
	}	
	{
		animMap 10 textures/sfx/r_flame2.tga textures/sfx/r_flame3.tga textures/sfx/r_flame4.tga textures/sfx/r_flame5.tga textures/sfx/r_flame6.tga textures/sfx/r_flame7.tga textures/sfx/r_flame8.tga textures/sfx/r_flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}	


	{
		map textures/sfx/r_flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6	
	}

}

textures/sfx/flameanim_green_pj
{
	//	*************************************************
	//	*      	Green Flame 				*
	//	*      October 2, 1999 Surface Light 2500    	*	
	//	*	Please Comment Changes			*
	//	*************************************************
	qer_editorimage textures/sfx/g_flame6.tga
	q3map_lightimage textures/sfx/g_flame6.tga
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm nolightmap
	cull none
	q3map_surfacelight 2500
	// texture changed to green flame.... PAJ
	{
		animMap 10 textures/sfx/g_flame1.tga textures/sfx/g_flame2.tga textures/sfx/g_flame3.tga textures/sfx/g_flame4.tga textures/sfx/g_flame5.tga textures/sfx/g_flame6.tga textures/sfx/g_flame7.tga textures/sfx/g_flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10	
	}	
	{
		animMap 10 textures/sfx/g_flame2.tga textures/sfx/g_flame3.tga textures/sfx/g_flame4.tga textures/sfx/g_flame5.tga textures/sfx/g_flame6.tga textures/sfx/g_flame7.tga textures/sfx/g_flame8.tga textures/sfx/g_flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}	
	{
		map textures/sfx/g_flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6	
	}
}


textures/sfx/xflame1side
{

	//	*************************************************
	//	*      	Yellow Flame Side			*
	//	*      	April 30 1999				*	
	//	*	Please Comment Changes			*
	//	*************************************************
	
		//q3map_surfacelight 500
		//q3map_lightimage textures/sfx/flame1.tga
		surfaceparm trans
		surfaceparm nomarks
		qer_editorimage textures/sfx/flame1.tga
		surfaceparm nolightmap
		cull none

	{
		animMap 10 textures/sfx/flame1.tga textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
		
	}	
	{
		animMap 10 textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga textures/sfx/flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}	


	{
		map textures/sfx/flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6	
	}

}


textures/sfx/flame1side
{

	//	*************************************************
	//	*      	Yellow Flame Side			*
	//	*      	April 30 1999				*	
	//	*	Please Comment Changes			*
	//	*************************************************
	
		surfaceparm trans
		surfaceparm nomarks
		surfaceparm nonsolid
	
		surfaceparm nolightmap
		cull none

	{
		animMap 10 textures/sfx/flame1.tga textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
		
	}	
	{
		animMap 10 textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga textures/sfx/flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}	


	{
		map textures/sfx/flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6	
	}

}



textures/sfx/flame2
{

	//	*************************************************
	//	*      	Yellow Flame Surface Light 5500		*
	//	*      	April 30 1999				*	
	//	*	Please Comment Changes			*
	//	*************************************************
	
		surfaceparm nomarks
		surfaceparm nolightmap
		cull none
		q3map_surfacelight 5500
		qer_editorimage textures/sfx/flame1.tga

	{
		animMap 10 textures/sfx/flame1.tga textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
		
	}	
	{
		animMap 10 textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga textures/sfx/flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}	

	{
		map textures/sfx/flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6	
	}

}


textures/sfx/xflame1
{

	//	*************************************************
	//	*      	Yellow Flame Surface Light 7500		*
	//	*      	March 30 1999 				*	
	//	*	Please Comment Changes			*
	//	*************************************************
	
	surfaceparm nomarks
	surfaceparm nolightmap
	cull none
	q3map_surfacelight 7500
	qer_editorimage textures/sfx/flame1.tga
	

	{
		animMap 10 textures/sfx/flame1.tga textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
		
	}	
	{
		animMap 10 textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga textures/sfx/flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}	


	{
		map textures/sfx/flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6	
	}

}


textures/sfx/xflamebright
{
	deformVertexes autoSprite2 
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm nolightmap
	cull none
	q3map_surfacelight 3000
	qer_editorimage textures/sfx/flame1.tga
	{
		animMap 10 textures/sfx/flame1.tga textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
	}	
	{
		animMap 10 textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga textures/sfx/flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}	
	{
		map textures/sfx/flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6	
	}
}


textures/sfx/flame1dark
{

	//	*************************************************
	//	*      	Yellow Flame used in q3test1				*
	//	*      	April 30 1999 Surface Light 7500	*	
	//	*	Please Comment Changes			*
	//	*************************************************
	
	qer_editorimage textures/sfx/flame1.tga
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm trans
	cull none
	q3map_surfacelight 2000
	

	{
		animMap 10 textures/sfx/flame1.tga textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
		
	}	
	{
		animMap 10 textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga textures/sfx/flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}	


	{
		map textures/sfx/flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6	
	}

}

textures/sfx/flame1_hell
{

	//	*************************************************
	//	*      	Yellow Flame used in q3test1				*
	//	*      	April 30 1999 Surface Light 7500	*	
	//	*	Please Comment Changes			*
	//	*************************************************
	
	qer_editorimage textures/sfx/flame1.tga
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm trans
	cull none
	q3map_surfacelight 600
	

	{
		animMap 10 textures/sfx/flame1.tga textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
		
	}	
	{
		animMap 10 textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga textures/sfx/flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}	


	{
		map textures/sfx/flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6	
	}

}

textures/sfx/pentagramfloor_red3test
{
	{
		map textures/sfx/pentagramfloor_red3test.tga
		rgbGen identity
	}
	{
		map $lightmap
		rgbGen identity
		blendfunc gl_dst_color gl_zero
	}
	{
		map textures/sfx/pentagramfloor_red3glowtest.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .5 .5 0 .5	
		depthfunc equal
	}
}

textures/sfx/goopq1metal7_98dhuge_pent
{
	{
		map textures/sfx/goopq1metal7_98dhuge_pent.tga
		rgbGen identity
	}
	{
		map $lightmap
		rgbGen identity
		blendfunc gl_dst_color gl_zero
	}
	{
		map textures/sfx/pentagramfloor_red3.glow.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .5 .5 0 .5	
		depthfunc equal
	}
}

textures/sfx/metaltechfloor02final_pent
{
	qer_editorimage textures/sfx/metaltechfloor02final_pent.tga
	{
		map textures/sfx/metaltechfloor02final_pent.tga
		rgbGen identity
	}
	{
		map $lightmap
		rgbGen identity
		blendfunc gl_dst_color gl_zero
	}
	{
		map textures/sfx/pentagramfloor_red_glow.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .5 .5 0 .5	
		depthfunc equal
	}
}

textures/sfx/blocks17final_pent
{
	qer_editorimage textures/sfx/blocks17final_pent.tga
	{
		map textures/sfx/blocks17final_pent.tga
		rgbGen identity
	}
	{
		map $lightmap
		rgbGen identity
		blendfunc gl_dst_color gl_zero
	}
	{
		map textures/sfx/pentagramfloor_red_glow.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .5 .5 0 .5	
		depthfunc equal
	}
}

textures/sfx/pentfloor_diamond2c
{
	qer_editorimage textures/sfx/pentfloor_diamond2c.tga
	{
		map textures/sfx/pentfloor_diamond2c.tga
		rgbGen identity
	}
	{
		map $lightmap
		rgbGen identity
		blendfunc gl_dst_color gl_zero
	}
	{
		map textures/sfx/pentagramfloor_red_glow.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .5 .5 0 .5	
		depthfunc equal
	}
}


textures/sfx/metalbridge06c_big_pentagram
{
	qer_editorimage textures/gothic_floor/metalbridge06c_big_pentagram.tga
	{
		map textures/gothic_floor/metalbridge06c_big_pentagram.tga
		rgbGen identity
	}
	{
		map $lightmap
		rgbGen identity
		blendfunc gl_dst_color gl_zero
	}
	{
		map textures/sfx/pentagramfloor_red_glow.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .5 .5 0 .5	
		depthfunc equal
	}
}

textures/sfx/largerblock3_pent
{
	qer_editorimage textures/gothic_floor/largerblock3_pent.tga
	{
		map textures/gothic_floor/largerblock3_pent.tga
		rgbGen identity
	}
	{
		map $lightmap
		rgbGen identity
		blendfunc gl_dst_color gl_zero
	}
	{
		map textures/sfx/pentagramfloor_red_glow.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .5 .5 0 .5	
		depthfunc equal
	}
}

textures/sfx/largerblock3b3_pent
{
	qer_editorimage textures/sfx/largerblock3b3_pent.tga
	{
		map textures/sfx/largerblock3b3_pent.tga
		rgbGen identity
	}
	{
		map $lightmap
		rgbGen identity
		blendfunc gl_dst_color gl_zero
	}
	{
		map textures/sfx/pentagramfloor_red_glow.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .5 .5 0 .5	
		depthfunc equal
	}
}


textures/sfx/gothgrate1
{
	surfaceparm	metalsteps		
	cull none

	// A GRATE OR GRILL THAT CAN BE SEEN FROM BOTH SIDES
	{
		map textures/sfx/gothgrate1.tga
		blendFunc GL_ONE GL_ZERO
		rgbGen identity
		alphaFunc GE128
		depthWrite
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
		depthFunc equal
	}
}

textures/sfx/blocks18ccomputer_offset
{

	//	*************************************************
	//	*      	Computer 8 on Blocks18c			*
	//	*      	March 18 1999				*	
	//	*	Please Comment Changes			*
	//	*************************************************
	// Created Mar.17.1999 (Xian)
	// This texture was originally created on a 512x512 version of blocks18c
	// The computer was placed in the middle and the section at 128,128 - 384,384
	// was used to create this texture


	surfaceparm nodamage
	q3map_lightimage textures/sfx/computer8.tga	
	q3map_surfacelight 200

	
	{
		map textures/sfx/blocks18ccomputer_offset.tga
		rgbGen identity
	}
	
	{
		map $lightmap
		rgbGen identity
		blendfunc gl_dst_color gl_zero
	}
	
	{
		map textures/sfx/computer8.tga
		blendfunc gl_one gl_one
		rgbGen wave sin .5 .3 0 3	
	}

	//	END


}

textures/sfx/computerblocks11b
{

	surfaceparm nodamage
	q3map_surfacelight 200

	
	{
		map textures/sfx/computerblocks11b.tga
		rgbGen identity
	}
	
	{
		map $lightmap
		blendfunc gl_dst_color gl_zero
		rgbGen identity
	}
	
	{
		animMap 2 textures/sfx/compscreen/letters1.tga textures/sfx/compscreen/letters2.tga textures/sfx/compscreen/letters3.tga textures/sfx/compscreen/letters5.tga  textures/sfx/compscreen/letters4.tga textures/sfx/compscreen/letters5.tga textures/sfx/compscreen/letters5.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 2
	}
	
	//{
	//	animMap 9 textures/sfx/compscreen/rotate0002.tga textures/sfx/compscreen/rotate0003.tga textures/sfx/compscreen/rotate0004.tga textures/sfx/compscreen/rotate0005.tga textures/sfx/compscreen/rotate0006.tga textures/sfx/compscreen/rotate0007.tga textures/sfx/compscreen/rotate0008.tga textures/sfx/compscreen/rotate0001.tga
	//	blendFunc GL_ONE GL_ONE
	//	rgbGen wave Sawtooth 0 1 0 9
	//}

	
}


textures/sfx/blocks18ccomputer
{

	//	*************************************************
	//	*      	Computer 8 on Blocks18c			*
	//	*      	March 18 1999				*	
	//	*	Please Comment Changes			*
	//	*************************************************

	
	surfaceparm nodamage
	q3map_surfacelight 200

	
	{
		map textures/sfx/blocks18ccomputer.tga
		rgbGen identity
	}
	
	{
		map $lightmap
		blendfunc gl_dst_color gl_zero
		rgbGen identity
	}
	
	{
		animMap 2 textures/sfx/compscreen/letters1.tga textures/sfx/compscreen/letters2.tga textures/sfx/compscreen/letters3.tga textures/sfx/compscreen/letters5.tga  textures/sfx/compscreen/letters4.tga textures/sfx/compscreen/letters5.tga textures/sfx/compscreen/letters5.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 2
	}
	
	//{
	//	animMap 9 textures/sfx/compscreen/rotate0002.tga textures/sfx/compscreen/rotate0003.tga textures/sfx/compscreen/rotate0004.tga textures/sfx/compscreen/rotate0005.tga textures/sfx/compscreen/rotate0006.tga textures/sfx/compscreen/rotate0007.tga textures/sfx/compscreen/rotate0008.tga textures/sfx/compscreen/rotate0001.tga
	//	blendFunc GL_ONE GL_ONE
	//	rgbGen wave Sawtooth 0 1 0 9
	//}

	
}


textures/sfx/computerblocks15
{

	//	*************************************************
	//	*      	Computer 8 on Blocks15			*
	//	*      	May 11 1999				*	
	//	*	Please Comment Changes			*
	//	*************************************************

	
	surfaceparm nodamage
	q3map_surfacelight 200

	
	{
		map textures/sfx/computerblocks15.tga
		rgbGen identity
	}
	
	{
		map $lightmap
		blendfunc gl_dst_color gl_zero
		rgbGen identity
	}
	
	{
		animMap 2 textures/sfx/compscreen/letters1.tga textures/sfx/compscreen/letters2.tga textures/sfx/compscreen/letters3.tga textures/sfx/compscreen/letters5.tga  textures/sfx/compscreen/letters4.tga textures/sfx/compscreen/letters5.tga textures/sfx/compscreen/letters5.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 2
	}
	
	//{
	//	animMap 9 textures/sfx/compscreen/rotate0002.tga textures/sfx/compscreen/rotate0003.tga textures/sfx/compscreen/rotate0004.tga textures/sfx/compscreen/rotate0005.tga textures/sfx/compscreen/rotate0006.tga textures/sfx/compscreen/rotate0007.tga textures/sfx/compscreen/rotate0008.tga textures/sfx/compscreen/rotate0001.tga
	//	blendFunc GL_ONE GL_ONE
	//	rgbGen wave Sawtooth 0 1 0 9
	//}	
}


textures/sfx/computer_blocks17
{

	//	*************************************************
	//	*      	Computer on Blocks17			*
	//	*      	May 28 1999				*	
	//	*	Please Comment Changes			*
	//	*************************************************

	
	surfaceparm nodamage
	q3map_surfacelight 200

	
	{
		map textures/sfx/computer_blocks17.tga
		rgbGen identity
	}
	
	{
		map $lightmap
		blendfunc gl_dst_color gl_zero
		rgbGen identity
	}
	
	{
		animMap 2 textures/sfx/compscreen/letters1.tga textures/sfx/compscreen/letters2.tga textures/sfx/compscreen/letters3.tga textures/sfx/compscreen/letters5.tga  textures/sfx/compscreen/letters4.tga textures/sfx/compscreen/letters5.tga textures/sfx/compscreen/letters5.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 2
	}
	
}


textures/sfx/computerpurptileb
{

	//	*************************************************
	//	*      	Computer 8 on computerpurptileb		*
	//	*      	March 18 1999				*	
	//	*	Please Comment Changes			*
	//	*************************************************

	//q3map_surfacelight 2000
	surfaceparm nodamage
	//q3map_lightimage textures/sfx/compscreen/morph0020.tga 
	q3map_surfacelight 200

	
	{
		map textures/sfx/computerpurptileb.tga
		rgbGen identity
	}
	
	{
		map $lightmap
		blendfunc gl_dst_color gl_zero
		rgbGen identity
	}
	

	{
		animMap 2 textures/sfx/compscreen/letters1.tga textures/sfx/compscreen/letters2.tga textures/sfx/compscreen/letters3.tga textures/sfx/compscreen/letters5.tga  textures/sfx/compscreen/letters4.tga textures/sfx/compscreen/letters5.tga textures/sfx/compscreen/letters5.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 2
	}
	
	//{
	//	animMap 9 textures/sfx/compscreen/rotate0002.tga textures/sfx/compscreen/rotate0003.tga textures/sfx/compscreen/rotate0004.tga textures/sfx/compscreen/rotate0005.tga textures/sfx/compscreen/rotate0006.tga textures/sfx/compscreen/rotate0007.tga textures/sfx/compscreen/rotate0008.tga textures/sfx/compscreen/rotate0001.tga
	//	blendFunc GL_ONE GL_ONE
	//	rgbGen wave Sawtooth 0 1 0 9
	//}

	
}

textures/sfx/diamond2cjumppad
{
	qer_editorimage textures/sfx/bouncepad01_diamond2cTGA.tga
	//q3map_surfacelight 2000
	surfaceparm nodamage
	q3map_lightimage textures/sfx/jumppadsmall.tga	
	q3map_surfacelight 500

	
	{
		map textures/sfx/bouncepad01_diamond2cTGA.tga
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
		clampmap textures/sfx/jumppadsmall.tga
		blendfunc gl_one gl_one
		tcMod stretch sin 1.2 .8 0 1.5
		rgbGen wave square .5 .5 .25 1.5
	}

	//	END
}
textures/sfx/blocks11bjumppad
{

	//q3map_surfacelight 2000
	surfaceparm nodamage
	q3map_lightimage textures/sfx/jumppadsmall.tga	
	q3map_surfacelight 400

	
	{
		map textures/sfx/blocks11bjumppad.tga
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
		clampmap textures/sfx/jumppadsmall.tga
		blendfunc gl_one gl_one
		tcMod stretch sin 1.2 .8 0 1.5
		rgbGen wave square .5 .5 .25 1.5
	}

	//	END
}

textures/sfx/bounce_metalbridge04
{

	//q3map_surfacelight 2000
	surfaceparm nodamage
	q3map_lightimage textures/sfx/jumppadsmall.tga	
	q3map_surfacelight 400

	
	{
		map textures/sfx/bounce_metalbridge04.tga
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
		clampmap textures/sfx/jumppadsmall.tga
		blendfunc gl_one gl_one
		tcMod stretch sin 1.2 .8 0 1.5
		rgbGen wave square .5 .5 .25 1.5
	}

	//	END
}

textures/sfx/bounce_largeblock3
{

	//q3map_surfacelight 2000
	surfaceparm nodamage
	q3map_lightimage textures/sfx/jumppadsmall.tga	
	q3map_surfacelight 400

	
	{
		map textures/sfx/bounce_largeblock3.tga
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
		clampmap textures/sfx/jumppadsmall.tga
		blendfunc gl_one gl_one
		tcMod stretch sin 1.2 .8 0 1.5
		rgbGen wave square .5 .5 .25 1.5
	}

	//	END
}

textures/sfx/bounce_largeblock3b
{

	//q3map_surfacelight 2000
	surfaceparm nodamage
	q3map_lightimage textures/sfx/jumppadsmall.tga	
	q3map_surfacelight 400

	
	{
		map textures/sfx/bounce_largeblock3b.tga
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
		clampmap textures/sfx/jumppadsmall.tga
		blendfunc gl_one gl_one
		tcMod stretch sin 1.2 .8 0 1.5
		rgbGen wave square .5 .5 .25 1.5
	}

	//	END
}

textures/sfx/bounce_dirt
{

	surfaceparm nodamage
	q3map_lightimage textures/sfx/jumppadsmall.tga	
	q3map_surfacelight 400

	
	{
		map textures/sfx/bounce_dirt.tga
		rgbGen identity
	}
	
	{
		map $lightmap
		rgbGen identity
		blendfunc filter
	}
	
	{
		map textures/sfx/bouncepad01b_layer1.tga
		blendfunc add
		rgbGen wave sin .5 .5 0 1.5	
	}

	{
		clampmap textures/sfx/jumppadsmall.tga
		blendfunc add
		tcMod stretch sin 1.2 .8 0 1.5
		rgbGen wave square .5 .5 .25 1.5
	}

	//	END
}


textures/sfx/bounce_concrete
{

	surfaceparm nodamage
	q3map_lightimage textures/sfx/jumppadsmall.tga	
	q3map_surfacelight 400

	
	{
		map textures/sfx/bounce_concrete.tga
		rgbGen identity
	}
	
	{
		map $lightmap
		rgbGen identity
		blendfunc filter
	}
	
	{
		map textures/sfx/bouncepad01b_layer1.tga
		blendfunc add
		rgbGen wave sin .5 .5 0 1.5	
	}

	{
		clampmap textures/sfx/jumppadsmall.tga
		blendfunc add
		tcMod stretch sin 1.2 .8 0 1.5
		rgbGen wave square .5 .5 .25 1.5
	}

	//	END
}



textures/sfx/bouncepad01_metalbridge06b
{

	//q3map_surfacelight 2000
	surfaceparm nodamage
	q3map_lightimage textures/sfx/jumppadsmall.tga	
	q3map_surfacelight 400

	
	{
		map textures/sfx/bouncepad01_metalbridge06b.tga
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
		clampmap textures/sfx/jumppadsmall.tga
		blendfunc gl_one gl_one
		tcMod stretch sin 1.2 .8 0 1.5
		rgbGen wave square .5 .5 .25 1.5
	}

	//	END
}


textures/sfx/bouncepad01bgoopy7_98d
{

	//q3map_surfacelight 2000
	surfaceparm nodamage
	q3map_lightimage textures/sfx/jumppadsmall.tga	
	q3map_surfacelight 400

	
	{
		map textures/sfx/bouncepad01bgoopy7_98d.tga
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
		clampmap textures/sfx/jumppadsmall.tga
		blendfunc gl_one gl_one
		tcMod stretch sin 1.2 .8 0 1.5
		rgbGen wave square .5 .5 .25 1.5
	}

	//	END
}
textures/sfx/bouncepad01b_metalbridge06c
{

	//q3map_surfacelight 2000
	surfaceparm nodamage
	q3map_lightimage textures/sfx/jumppadsmall.tga	
	q3map_surfacelight 400

	
	{
		map textures/sfx/bouncepad01b_metalbridge06c.tga
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
		clampmap textures/sfx/jumppadsmall.tga
		blendfunc gl_one gl_one
		tcMod stretch sin 1.2 .8 0 1.5
		rgbGen wave square .5 .5 .25 1.5
	}

	//	END
}

textures/sfx/bounce_xq1metalbig
{

	//q3map_surfacelight 2000
	surfaceparm nodamage
	q3map_lightimage textures/sfx/jumppadsmall.tga	
	q3map_surfacelight 400

	
	{
		map textures/sfx/bounce_xq1metalbig.tga
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
		clampmap textures/sfx/jumppadsmall.tga
		blendfunc gl_one gl_one
		tcMod stretch sin 1.2 .8 0 1.5
		rgbGen wave square .5 .5 .25 1.5
	}

	//	END
}

textures/sfx/bouncepad01block18b
{

	//	*************************************************
	//	*      	Bounce Pad on Blocks18b			*
	//	*      	March 10 1999				*	
	//	*	Please Comment Changes			*
	//	*************************************************

	//q3map_surfacelight 2000
	surfaceparm nodamage
	q3map_lightimage textures/sfx/jumppadsmall.tga	
	q3map_surfacelight 2000

	
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
		clampmap textures/sfx/jumppadsmall.tga
		blendfunc gl_one gl_one
		tcMod stretch sin 1.2 .8 0 1.5
		rgbGen wave square .5 .5 .25 1.5
	}

	//	END
}

textures/sfx/bouncepad01_block17
{

	//	*************************************************
	//	*      	Bounce Pad on Blocks17			*
	//	*      	March 10 1999				*	
	//	*	Please Comment Changes			*
	//	*************************************************

	//q3map_surfacelight 2000
	surfaceparm nodamage
	q3map_lightimage textures/sfx/jumppadsmall.tga	
	q3map_surfacelight 1000

	
	{
		map textures/sfx/bouncepad01_block17.tga
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
		clampmap textures/sfx/jumppadsmall.tga
		blendfunc gl_one gl_one
		tcMod stretch sin 1.2 .8 0 1.5
		rgbGen wave square .5 .5 .25 1.5
	}

	//	END
}


textures/sfx/bouncepad1q1metal7_99
{

	//	*************************************************
	//	*      	Bounce Pad on q1metal97			*
	//	*      	March 10 1999				*	
	//	*	Please Comment Changes			*
	//	*************************************************

	//q3map_surfacelight 2000
	surfaceparm nodamage
	q3map_lightimage textures/sfx/jumppadsmall.tga	
	q3map_surfacelight 2000

	
	{
		map textures/sfx/bouncepad1q1metal7_99.tga
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
		clampmap textures/sfx/jumppadsmall.tga
		blendfunc gl_one gl_one
		tcMod stretch sin 1.2 .8 0 1.5
		rgbGen wave square .5 .5 .25 1.5
	}

	//	END
}

textures/sfx/redpadblocks17
{
	qer_editorimage textures/sfx/redpad.tga
	q3map_lightimage textures/sfx/redpad.blend.tga
	q3map_surfacelight 2000
	//q3map_surfacelight 2000
	surfaceparm nodamage
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/gothic_block/blocks17.tga
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		clampmap textures/sfx/redpad.blend.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.75 0.25 0 1
		tcMod stretch sin 0.80 0.20 0 1
	}
}

textures/sfx/0
{
	surfaceparm nolightmap
	surfaceparm nonsolid
	cull twosided
	{
		map textures/sfx/0.tga
		blendfunc GL_ONE GL_ONE
	}
}
textures/sfx/1
{
	surfaceparm nolightmap
	surfaceparm nonsolid
	cull twosided
	{
		map textures/sfx/1.tga
		blendfunc GL_ONE GL_ONE
	}
}
textures/sfx/2
{
	surfaceparm nolightmap
	surfaceparm nonsolid
	cull twosided
	{
		map textures/sfx/2.tga
		blendfunc GL_ONE GL_ONE
	}
}
textures/sfx/3
{
	surfaceparm nolightmap
	surfaceparm nonsolid
	cull twosided
	{
		map textures/sfx/3.tga
		blendfunc GL_ONE GL_ONE
	}
}
textures/sfx/4
{
	surfaceparm nolightmap
	surfaceparm nonsolid
	cull twosided
	{
		map textures/sfx/4.tga
		blendfunc GL_ONE GL_ONE
	}
}
textures/sfx/5
{
	surfaceparm nolightmap
	surfaceparm nonsolid
	cull twosided
	{
		map textures/sfx/5.tga
		blendfunc GL_ONE GL_ONE
	}
}
textures/sfx/6
{
	surfaceparm nolightmap
	surfaceparm nonsolid
	cull twosided
	{
		map textures/sfx/6.tga
		blendfunc GL_ONE GL_ONE
	}
}
textures/sfx/7
{
	surfaceparm nolightmap
	surfaceparm nonsolid
	cull twosided
	{
		map textures/sfx/7.tga
		blendfunc GL_ONE GL_ONE
	}
}
textures/sfx/8
{
	surfaceparm nolightmap
	surfaceparm nonsolid
	cull twosided
	{
		map textures/sfx/8.tga
		blendfunc GL_ONE GL_ONE
	}
}
textures/sfx/9
{
	surfaceparm nolightmap
	surfaceparm nonsolid
	cull twosided
	{
		map textures/sfx/9.tga
		blendfunc GL_ONE GL_ONE
	}
}







textures/sfx/goopq1metal7_98d_pent_256
{	
	{
		map textures/gothic_floor/goopq1metal7_98d.tga
	}
	{
		map textures/sfx/pentfloor_256.tga
		blendfunc gl_src_alpha gl_one_minus_src_alpha
	}
	//{
	//	map $lightmap
	//	blendfunc gl_dst_color gl_zero
//		rgbGen identity
	//}
	//{
	//	map textures/sfx/pentfloor.glow.tga
	//	blendfunc gl_one gl_one
	//	rgbGen wave sin .5 .5 0 .5	
	//}
}

textures/sfx/pentagramfloor_red
{

	//	*************************************************
	//	*      	PENTAGRAM EFFECT			*
	//	*      	March 1 1999				*	
	//	*	Please Comment Changes			*
	//	*************************************************

	polygonOffset
			
		{
			map textures/sfx/pentagramfloor_red.tga
			alphafunc GT0
			depthWrite
		}
		
		{
			map $lightmap
			rgbGen identity
			blendfunc GL_DST_COLOR GL_ZERO
			depthfunc equal
		}

		{
			map textures/sfx/pentagramfloor_red_glow.tga
			blendFunc GL_ONE GL_ONE
			rgbGen wave sin .5 .5 0 .5	
//			tcMod rotate 40
			depthfunc equal
		}

	//	END
}

textures/sfx/goopq1metal7_98dnodamage
{
	qer_editorimage textures/gothic_floor/goopq1metal7_98d.tga
	surfaceparm nodamage
}

textures/sfx/gimmickdiamondarrow
{
	qer_editorimage textures/sfx/goopq1metal7_98darrow.tga
	q3map_lightimage textures/sfx/pitted_rust3arrow.blend.tga
	//q3map_surfacelight 2000
	q3map_surfacelight 500
	{
		rgbGen identity
		map $lightmap
	}
	{
		map textures/base_floor/diamond2c.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/sfx/pitted_rust3arrow.blend.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.75 0.25 0 0.5
		tcmod scroll 0 1
	}
}
textures/sfx/goopq1metal7_98darrow
{
	qer_editorimage textures/sfx/goopq1metal7_98darrow.tga
	q3map_lightimage textures/sfx/pitted_rust3arrow.blend.tga
	q3map_surfacelight 2000
	q3map_surfacelight 500
	{
		rgbGen identity
		map $lightmap
	}
	{
		map textures/gothic_floor/goopq1metal7_98dlarge.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/sfx/pitted_rust3arrow.blend.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.75 0.25 0 0.5
		tcmod scroll 0 1
	}
}

textures/sfx/blocks11b_himom
{
	{
		map $lightmap
		rgbGen identity
	}
	{
		rgbGen identity
		map textures/gothic_block/blocks11b.tga
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/sfx/himom.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.75 0.25 0 0.5
	}
}

textures/sfx/pittedrust3stripes
{
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/gothic_trim/pitted_rust3.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/sfx/orangestripes.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.75 0.25 0 0.5
	}
}


textures/sfx/q1metal7_98dstripe
{
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/gothic_floor/q1metal7_98d.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/sfx/orangestripes.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.75 0.25 0 0.5
	}
}

textures/sfx/goopq1metal7_98dstripe
{
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/gothic_floor/goopq1metal7_98d.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/sfx/orangestripes.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.75 0.25 0 0.5
	}
}


textures/sfx/pentagramfloor_rotating
{
	qer_editorimage textures/sfx/pentagramfloor.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/gothic_floor/xhugefloor2.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
//		map textures/sfx/pentagramfloor.red.blend.tga
		clampmap textures/sfx/pentagramfloor.red.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.75 0.25 0 0.5
		tcMod rotate -60
	}
}

textures/sfx/pentagramfloor_rotating2
{
	qer_editorimage textures/sfx/pentagramfloor.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/gothic_floor/q1metalhuge.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		clampmap textures/sfx/pentagramfloor.red.blend.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.75 0.25 0 0.5
		tcMod rotate -60
	}
}

textures/sfx/pentagramfloor_q1metal7_99
{
	qer_editorimage textures/sfx/pentagramfloor.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/gothic_floor/q1metalhuge.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		clampmap textures/sfx/pentagramfloor.red.blend.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.75 0.25 0 0.5
		tcMod rotate -60
	}
}

textures/sfx/pentagramfloor
{
	q3map_surfacelight 250
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/gothic_floor/xhugefloor2.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		clampmap textures/sfx/pentagramfloor.red.blend.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.75 0.25 0 0.5
		tcMod rotate -60
	}
}

textures/sfx/q1met_redpad
{
	q3map_surfacelight 1000
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/sfx/q1met_redpad.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/sfx/q1met_redpad.blend.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.5 0.5 0 .2
	}
}
textures/sfx/redgoal
{
	surfaceparm nolightmap
	//surfaceparm nonsolid
	surfaceparm trans
	cull twosided
	{
		map textures/sfx/redgoal.tga
		tcGen environment
		blendfunc GL_ONE GL_ONE
		tcMod turb 0 0.25 0 0.5
	}
}


textures/sfx/bluegoal
{
	surfaceparm nolightmap
	surfaceparm nonsolid
	surfaceparm trans
	cull twosided
	{
		map textures/sfx/powerupshit.tga
		tcGen environment
		blendfunc GL_ONE GL_ONE
		tcMod turb 0 0.25 0 0.5
	}
}

textures/sfx/xdensegreyfog
{
// Don't mess with this entry please.
// I use this for my dense low-lying fog
// -Xian
surfaceparm	trans
surfaceparm	nonsolid
surfaceparm	fog
surfaceparm	nolightmap
qer_nocarve
fogparms ( 0.7 0.7 0.7 ) 1700
}

textures/sfx/xlightgreyfog
{
qer_editorimage textures/sfx/xdensegreyfog.tga
surfaceparm	trans
surfaceparm	nonsolid
surfaceparm	fog
surfaceparm	nolightmap
qer_nocarve
fogparms ( 0.7 0.7 0.7 ) 1400

}

textures/sfx/xfinalfog
{
qer_editorimage textures/sfx/xdensegreyfog.tga
surfaceparm	trans
surfaceparm	nonsolid
surfaceparm	fog
surfaceparm	nolightmap
qer_nocarve
fogparms ( 0.7 0.7 0.7 ) 3000

}


textures/sfx/xfiretest1
{
	qer_editorimage textures/sfx/firetestb.tga
	//surfaceparm trans
	surfaceparm noimpact
	//surfaceparm nonsolid
	surfaceparm nolightmap
	q3map_surfacelight 1000
	tesssize 64
	
	
	{
	map textures/sfx/firetestb.tga
	blendfunc gl_one gl_zero
	tcMod turb .2 .2 .1 1
	tcMod scale .25 .25
	tcMod scroll .5 1.5
	}

	{
	map textures/sfx/firetest.tga
	blendFunc GL_DST_COLOR GL_SRC_ALPHA
	tcMod turb .2 .1 .1 1
	tcMod scale .1 .1
	tcMod scroll 0 1.0
	}
	
	{
	map textures/sfx/firetest2.tga
	blendFunc GL_DST_COLOR GL_ONE
	tcMod turb .1 .1 .5 .5
	tcMod scroll 1 5.0
	}
}
textures/sfx/xfiretest2
{
	qer_editorimage textures/sfx/firetestb.tga
	surfaceparm noimpact
	surfaceparm nolightmap
	q3map_surfacelight 1000
	tesssize 64
	{
	map textures/sfx/firetest2.tga
	blendfunc gl_one gl_zero
	tcMod turb .2 .2 .1 1
	tcMod scale .25 .25
	tcMod scroll .5 1.5
	}

	{
	map textures/sfx/firetest.tga

	blendFunc GL_DST_COLOR GL_SRC_ALPHA
	tcMod turb .2 .1 .1 1
	tcMod scale .1 .1
	tcMod scroll 0 1.0
	}
	
	{

	map textures/sfx/firetestb.tga
	blendFunc GL_DST_COLOR GL_ONE
	tcMod turb .1 .1 .5 .5
	tcMod scroll 1 5.0
	}
}

textures/sfx/blackness
{	
	surfaceparm nolightmap
	surfaceparm noimpact
	surfaceparm nomarks
	{
		map textures/sfx/blackness.tga
	}
}

textures/sfx/surface6jumppad
{
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/base_stone/surface6.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/sfx/surface6jumppad.blend.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.5 0.5 0 .2
//		blendFunc GL_ONE GL_ONE
//		rgbGen wave sin 0.75 0.25 0 .2
//		tcMod stretch sin 0.80 0.20 0 1
	}
}

textures/sfx/powerupshit
{
	surfaceparm nolightmap
	surfaceparm nonsolid
	cull twosided
	{
		map textures/sfx/powerupshit.tga
		tcGen environment
		blendfunc GL_ONE GL_ONE
		tcMod turb 0 0.25 0 0.5
	}
}


textures/sfx/redstripe
{
	surfaceparm nolightmap	
	{
		map textures/sfx/redstripe.tga
		blendFunc GL_ONE GL_ZERO
		rgbGen wave sin 0.75 0.25 0 .7
	}
}
textures/sfx/yellowstripe
{
	surfaceparm nolightmap	
	{
		map textures/sfx/yellowstripe.tga
		blendFunc GL_ONE GL_ZERO
		rgbGen wave sin 0.75 0.25 0 .7
	}
}

textures/sfx/bluestripe
{
	surfaceparm nolightmap
	{
		map textures/sfx/bluestripe.tga
		blendFunc GL_ONE GL_ZERO
		rgbGen wave sin 0.75 0.25 0 .7

	}
}

textures/sfx/xgoopjumpypad
{
	q3map_surfacelight 100
	q3map_lightimage textures/sfx/surface6jumppad.blend.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/sfx/xgoopq1metal.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		clampmap textures/sfx/surface6jumppad.blend.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.75 0.25 0 .2
		tcMod stretch sin 0.80 0.20 0 1
	}
}

textures/sfx/xgoopy_pentagram
{
	q3map_surfacelight 250
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/sfx/xgoopq1metal.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		clampmap textures/sfx/pentagramfloor.red.blend.tga
		blendFunc GL_ONE GL_ONE
		tcMod rotate -60
	}
}
textures/sfx/firetestnolight
{
	surfaceparm noimpact
	surfaceparm nolightmap
	tesssize 64
	
	
	{
	map textures/sfx/firetestb.tga
	blendfunc GL_ONE GL_ZERO
	tcMod turb .2 .2 .1 1
	tcMod scale .25 .25
	tcMod scroll .5 1.5
	}

	{
	map textures/sfx/firetest.tga
	blendFunc GL_DST_COLOR GL_SRC_ALPHA
	tcMod turb .2 .1 .1 1
	tcMod scale .1 .1
	tcMod scroll 0 1.0
	}
	
	{
	map textures/sfx/firetest2.tga
	blendFunc GL_DST_COLOR GL_ONE
	tcMod turb .1 .1 .5 .5
	tcMod scroll 1 5.0
	}

//	END
}
textures/sfx/firetest2
{
	surfaceparm noimpact
	surfaceparm nolightmap
	q3map_surfacelight 1000
	tesssize 64
	
	
	{
	map textures/sfx/firetestb.tga
	blendfunc GL_ONE GL_ZERO
	tcMod turb .2 .2 .1 1
	tcMod scale .25 .25
	tcMod scroll .5 1.5
	}

	{
	map textures/sfx/firetest.tga
	blendFunc GL_DST_COLOR GL_SRC_ALPHA
	tcMod turb .2 .1 .1 1
	tcMod scale .1 .1
	tcMod scroll 0 1.0
	}
	
	{
	map textures/sfx/firetest2.tga
	blendFunc GL_DST_COLOR GL_ONE
	tcMod turb .1 .1 .5 .5
	tcMod scroll 1 5.0
	}

//	END
}

textures/sfx/firewall
{

	//	*************************************************
	//	*      	Solid Fire Wall				*
	//	*      	Feb					*	
	//	*	Please Comment Changes			*
	//	*************************************************

	surfaceparm nonsolid
	surfaceparm nolightmap
	q3map_surfacelight 6000
	tesssize 64
	{
	map textures/sfx/firetestb.tga
	blendfunc GL_ONE GL_ZERO
	tcMod turb .2 .2 .1 1
	tcMod scale .25 .25
	tcMod scroll .5 1.5
	}

	{
	map textures/sfx/firetest.tga
	blendFunc GL_DST_COLOR GL_SRC_ALPHA
	tcMod turb .2 .1 .1 1
	tcMod scale .1 .1
	tcMod scroll 0 1.0
	}
	
	{
	map textures/sfx/firetest2.tga
	blendFunc GL_DST_COLOR GL_ONE
	tcMod turb .1 .1 .5 .5
	tcMod scroll 1 5.0
	}
}

textures/sfx/xq1metalbig_jumppad
{
	surfaceparm nodamage
//	q3map_surfacelight 250
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/gothic_floor/q1metal7_98d_256x256.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		clampmap textures/sfx/surface6jumppad.blend.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.75 0.25 0 .2
		tcMod stretch sin 0.80 0.20 0 1
	}
}

textures/sfx/blocks11b_jumppad
{
	surfaceparm nodamage
//	q3map_surfacelight 250
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/gothic_block/blocks11b.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		clampmap textures/sfx/jumppad.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.75 0.25 0 .2
		tcMod stretch sin 0.80 0.20 0 1
	}
}

textures/sfx/teslacoil
{
	cull none
	q3map_lightimage textures/sfx/tesla1.tga	
	q3map_surfacelight 100

	{
		map $lightmap
		tcgen environment
		blendfunc filter
	}
	
	{
		map textures/sfx/tesla1.tga
		blendfunc add
		rgbgen wave sawtooth 0 1 0 5
		tcmod scale 1 .5
		tcmod turb 0 .1 0 1
		tcMod scroll -1 -1
	}
 
	
	
	{
		map textures/sfx/electricslime.tga
		blendfunc add
		rgbgen wave sin 0 .5 0 1
		tcmod scale .5 .5
		tcmod turb 0 .1 0 1
		tcmod rotate 180
		tcmod scroll -1 -1
	}

	{
		map textures/sfx/cabletest2.tga
		blendfunc blend
	}

	
}

textures/sfx/teslacoil2
{
	qer_editorimage textures/sfx/tesla1.tga
	cull none
	deformVertexes wave 100 triangle 0 5 1 1


	{
		map textures/sfx/lightningcoil.tga
		blendfunc GL_ONE GL_ONE
		rgbGen wave sin .5 0 0 0
		tcMod scroll 5 5
		
	}
	
}

textures/sfx/teslacoil3
{
	cull none
	q3map_lightimage textures/sfx/tesla1b.tga	
	q3map_surfacelight 100
	qer_editorimage textures/sfx/teslacoil.tga
	surfaceparm nolightmap
	
	

	{
		map textures/sfx/cabletest2.tga
		blendfunc blend
	}

	
	{
		map textures/sfx/electricslime2.tga
		blendfunc add
		rgbGen wave square .25 .25 0 2.5
		tcmod scale 1 1
		tcMod scroll 1 1
	}



	{
		map textures/sfx/tesla1b.tga
		blendfunc add
		rgbgen wave square 0 1 0 3
		tcmod scale 1 1
		tcMod scroll -2 1
	}
	
}



textures/sfx/teslacoiltrans
{
	qer_editorimage textures/sfx/tesla1.tga
	surfaceparm trans
	surfaceparm nonsolid
	cull none
	deformVertexes wave 100 triangle 0 5 1 1


	{
		map textures/sfx/lightningcoil.tga
		blendfunc GL_ONE GL_ONE
		rgbGen wave sin .5 0 0 0
		tcMod scroll 5 5
		
	}
	
}


textures/sfx/pentagramfloor
{
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/sfx/pentagramfloor.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/sfx/pentagramfloor.red.blend.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.5 0.5 0 .5
	}
}

textures/sfx/spawnpad
{
// Jan.29.1999 -- Xian commented out q3map_surfacelight 3500
//	q3map_surfacelight 3500

	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/gothic_block/blocks17.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/sfx/spawnpad.blend.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.75 0.25 0 .2
//		tcMod stretch sin 0.80 0.20 0 1
	}
}
textures/sfx/q1metal7_98d_pent_256
{
	q3map_surfacelight 250
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/sfx/q1metal7_98d_pent_256.tga
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/sfx/pentagramfloor_256x256.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.5 0.5 0 .5
	}
}


//Tim's fog for map tim_dm1
textures/sfx/fog_timdm1
{
surfaceparm	trans
surfaceparm	nonsolid
surfaceparm	fog
surfaceparm	nolightmap
//fogparms ( .615 .309 0 ) 1800
fogparms ( .615 .309 0 ) 616
}

//Tim's fog for map tim_dm8
textures/sfx/fog_timdm8
{
surfaceparm	trans
surfaceparm	nonsolid
surfaceparm	fog
surfaceparm	nolightmap

fogparms ( .211 .231 .094 ) 250
}

//Tim's fog for map tim_dm2
textures/sfx/fog_timdm2
{
qer_editorimage textures/sfx/fog_grey.tga
surfaceparm	trans
surfaceparm	nonsolid
surfaceparm	fog
surfaceparm	nolightmap

fogparms ( .615 .309 0 ) 356
}

//Tim's fog for map tim_ctf1
textures/sfx/fog_timctf1
{
qer_editorimage textures/sfx/fog_grey.tga
surfaceparm	trans
surfaceparm	nonsolid
surfaceparm	fog
surfaceparm	nolightmap

fogparms ( .729 .729 .780 ) 464

}

//Paul's fog for map pj_dm4
textures/sfx/fog_pjdm1
{
qer_editorimage textures/sfx/fog_grey.tga
surfaceparm	trans
surfaceparm	nonsolid
surfaceparm	fog
surfaceparm	nolightmap

fogparms ( .733 .737 .620 ) 300

}

//Proto_hellfog
textures/sfx/proto_hellfog
{
qer_editorimage textures/sfx/proto_hellfog.tga
surfaceparm	trans
surfaceparm	nonsolid
surfaceparm	fog
surfaceparm	nolightmap

fogparms ( .275 .012 0 ) 500
}


//Tim's fog for q3test1
textures/sfx/fog_intel
{
qer_editorimage textures/sfx/fog_timdm1.tga
surfaceparm	trans
surfaceparm	nonsolid
surfaceparm	fog
surfaceparm	nolightmap

fogparms ( .75 .38 0 ) 800
}

//Paul's gray fog for q3dm10
textures/sfx/fog_q3dm10
{
qer_editorimage textures/sfx/fog_timdm1.tga
surfaceparm	trans
surfaceparm	nonsolid
surfaceparm	fog
surfaceparm	nolightmap

fogparms ( 0.75 0.38 0 ) 128
}



textures/sfx/diamondredpad
{
	q3map_surfacelight 1500
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/base_floor/diamond2c.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		clampmap textures/sfx/redpad.blend.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.75 0.25 0 1
		tcMod stretch sin 0.80 0.20 0 1
	}
}

textures/sfx/hellfog_tim_dm14
//**************************************
//New death fog that must be in all maps with death fog
//**************************************
{
	qer_editorimage textures/sfx/hellfog.tga
	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm 	nodrop
	surfaceparm 	nolightmap
	q3map_globaltexture
	q3map_surfacelight 50
	fogparms ( .55 .11 .1 ) 128
	
	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale -.05 -.05
		tcmod scroll .01 -.01
		rgbgen identity
	}

	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale .05 .05
		tcmod scroll .01 -.01
		rgbgen identity
	}


}

textures/sfx/kc_hellfog_1k
//**************************************
//New death fog that must be in all maps with death fog
//**************************************
{
	qer_editorimage textures/sfx/hellfog.tga
	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm 	nodrop
	surfaceparm 	nolightmap
	q3map_globaltexture
	q3map_surfacelight 300
	q3map_lightsubdivide 32
	fogparms ( .5 .12 .1 ) 225

	
	
	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale -.05 -.05
		tcmod scroll .01 -.01
		rgbgen identity
	}

	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale .05 .05
		tcmod scroll .01 -.01
		rgbgen identity
	}

}



textures/sfx/hellfog_1k
//**************************************
//New death fog that must be in all maps with death fog
//**************************************
{
	qer_editorimage textures/sfx/hellfog.tga
	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm 	nodrop
	surfaceparm 	nolightmap
	q3map_globaltexture
	q3map_surfacelight 200
	fogparms ( .5 .12 .1 ) 300

	
	
	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale -.05 -.05
		tcmod scroll .01 -.01
		rgbgen identity
	}

	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale .05 .05
		tcmod scroll .01 -.01
	}
}





textures/sfx/hellfog
//**************************************
//New death fog that must be in all maps with death fog
//**************************************
{
	qer_editorimage textures/sfx/hellfog.tga
	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm 	nodrop
	surfaceparm 	nolightmap
	q3map_globaltexture
	q3map_surfacelight 100
	fogparms ( .55 .11 .1 ) 300

	
	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale -.05 -.05
		tcmod scroll .01 -.01
		rgbgen identity
	}

	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale .05 .05
		tcmod scroll .01 -.01
		rgbgen identity
	}

}

textures/sfx/constfog
{
	qer_editorimage textures/sfx/hellfog.tga
	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm	nolightmap
	surfaceparm nodrop
	q3map_surfacelight 100
	fogparms ( .8 .8 .8 ) 100
}


textures/sfx/testconstantfog
{
	qer_editorimage textures/sfx/hellfog.tga
	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm	nolightmap
	surfaceparm nodrop
	q3map_surfacelight 100
	fogparms ( 0 1 0 ) 300
}

textures/sfx/q3tourney3fog
//**************************************
//New death fog that must be in all maps with death fog
//**************************************
{
	qer_editorimage textures/sfx/hellfog.tga
	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm 	nodrop
	surfaceparm 	nolightmap
	q3map_globaltexture
	q3map_surfacelight 100
	fogparms ( .55 .11 .1 ) 384
	
	

	
	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale -.05 -.05
		tcmod scroll .01 -.01
		rgbgen identity
	}

	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale .05 .05
		tcmod scroll .01 -.01
		rgbgen identity
	}
		
}

textures/sfx/q3dm14fog
//**************************************
//New death fog that must be in all maps with death fog
//**************************************
{
	qer_editorimage textures/sfx/hellfog.tga
	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm 	nodrop
	surfaceparm 	nolightmap
	q3map_globaltexture
	q3map_surfacelight 100
	fogparms ( .55 .11 .1 ) 256
	
	

	
	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale -.05 -.05
		tcmod scroll .01 -.01
		rgbgen identity
	}

	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale .05 .05
		tcmod scroll .01 -.01
		rgbgen identity
	}
		
}


textures/sfx/q3dm9fog
//**************************************
//New death fog that must be in all maps with death fog
//**************************************
{
	qer_editorimage textures/sfx/hellfog.tga
	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm 	nodrop
	surfaceparm 	nolightmap
	q3map_globaltexture
	q3map_surfacelight 100
	fogparms ( .55 .11 .1 ) 256
	
	

	
	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale -.05 -.05
		tcmod scroll .01 -.01
		rgbgen identity
	}

	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale .05 .05
		tcmod scroll .01 -.01
		rgbgen identity
	}
		
}


textures/sfx/hellfogdense
//**************************************
//New death fog that must be in all maps with death fog
//**************************************
{
	qer_editorimage textures/sfx/hellfog.tga
	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm 	nodrop
	surfaceparm 	nolightmap
	q3map_globaltexture
	q3map_surfacelight 100
	fogparms ( .55 .11 .1 ) 128
	
	

	
	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale -.05 -.05
		tcmod scroll .01 -.01
		rgbgen identity
	}

	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale .05 .05
		tcmod scroll .01 -.01
		rgbgen identity
	}
		
}

//textures/sfx/pj_dm9_fog2
//  created 6_23_99
//{
//	qer_editorimage textures/sfx/hellfog.tga
//	surfaceparm	trans
//	surfaceparm	nonsolid
//	surfaceparm	fog
//	surfaceparm	nolightmap
//	surfaceparm nodrop
//	q3map_surfacelight 60
//	fogparms ( .5 .5 .5 ) 32
//}
textures/sfx/pj_a1_fog1
//  created 7_06_99
{
	qer_editorimage textures/sfx/fog_pjteam1.tga
	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm	nolightmap
	surfaceparm nodrop
	q3map_surfacelight 30
	fogparms ( .5 .5 .5 ) 512
}
textures/sfx/pitted_rust3arrowbase
{
	qer_editorimage textures/sfx/pitted_rust3arrow.tga
	q3map_lightimage textures/sfx/pitted_rust3arrow.blend.tga
	q3map_surfacelight 2000
//	q3map_surfacelight 500
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/sfx/pitted_rust3arrow.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/sfx/pitted_rust3arrow.blend.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.5 0.5 0 .2
	}
}
textures/sfx/pitted_rust3arrowblocks
{
	qer_editorimage textures/sfx/pitted_rust3arrow.tga
	q3map_lightimage textures/sfx/pitted_rust3arrow.blend.tga
	q3map_surfacelight 2000
//	q3map_surfacelight 500
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/gothic_block/blocks17.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/sfx/pitted_rust3arrow.blend.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.75 0.25 0 1
	}
}
textures/sfx/q1teleporter
{
	qer_editorimage textures/sfx/q1teleporter.tga
	q3map_surfacelight 500
	//light 1
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	tesssize 128
	cull disable
	deformVertexes wave 100 sin 3 2 .1 0.1
	
	{
		map textures/sfx/q1teleporter.tga
		tcMod turb 0 .3 0 .2
	}
	
}

//fog for map mkc_dm4
textures/sfx/fog_mkcdm4
{
qer_editorimage textures/sfx/fog_grey.tga

surfaceparm	trans
surfaceparm	nonsolid
surfaceparm	fog
surfaceparm	nolightmap

fogparms ( .5 0 0 ) 96
}

textures/sfx/mkc_bigflame
{

		surfaceparm trans
		surfaceparm nomarks
		surfaceparm nonsolid
		qer_editorimage textures/sfx/flame1.tga
		q3map_surfacelight 500
		surfaceparm nolightmap
		cull none

	{
		animMap 10 textures/sfx/flame1.tga textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
		
	}	
	{
		animMap 10 textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga textures/sfx/flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}	


	{
		map textures/sfx/flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6	
	}

}

textures/sfx/mkc_fog_tdm3
{
	qer_editorimage textures/sfx/hellfog.tga
	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm 	nodrop
	surfaceparm 	nolightmap
	q3map_globaltexture
	fogparms ( 0.3 0.2 0.2 ) 320
	
	
	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale -.05 -.05
		tcmod scroll .01 -.01
		rgbgen identity
	}

	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale .05 .05
		tcmod scroll .01 -.01
		rgbgen identity
	}
}

textures/sfx/mkc_fog_ctfred
{
	qer_editorimage textures/sfx/fog_timdm1.tga
	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm 	nodrop
	surfaceparm 	nolightmap
	q3map_globaltexture
	fogparms ( 0.3 0.2 0.2 ) 320

	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale -.05 -.05
		tcmod scroll .01 -.01
		rgbgen identity
	}

	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale .05 .05
		tcmod scroll .01 -.01
		rgbgen identity
	}
}

textures/sfx/mkc_fog_ctfblue
{
	qer_editorimage textures/sfx/fog_timdm1.tga
	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm 	nodrop
	surfaceparm 	nolightmap
	q3map_globaltexture
	fogparms ( 0.2 0.2 0.25 ) 320
	
	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale -.05 -.05
		tcmod scroll .01 -.01
		rgbgen identity
	}

	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale .05 .05
		tcmod scroll .01 -.01
		rgbgen identity
	}
}

textures/sfx/mkc_fog_ctfblue2
{
	qer_editorimage textures/sfx/fog_timdm1.tga
	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm 	nodrop
	surfaceparm 	nolightmap
	q3map_globaltexture
	fogparms ( 0.1 0.1 0.3 ) 320

	q3map_surfacelight 40
	
	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale -.05 -.05
		tcmod scroll .01 -.01
		rgbgen identity
	}

	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		tcmod scale .05 .05
		tcmod scroll .01 -.01
		rgbgen identity
	}
}


textures/sfx/bugmirror
{
	qer_editorimage textures/sfx/mirrorkc.tga
	surfaceparm nolightmap
	portal
	{
		map textures/common/mirror1.tga
		blendfunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		depthWrite
	}
	{
		map textures/sfx/mirrorkc.tga
		blendFunc add	
		depthfunc equal
	}
}


textures/sfx/pureblack
{
	qer_editorimage textures/skies/blacksky.tga
	surfaceparm noimpact
	surfaceparm nolightmap
	{
		map textures/skies/blacksky.tga
	}
}