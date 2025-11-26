
textures/liquids/protolava
{
	
	q3map_globaltexture
	surfaceparm trans
	//surfaceparm nonsolid
	surfaceparm noimpact
	surfaceparm lava
	surfaceparm nolightmap
	q3map_surfacelight 600
	cull disable
	
	tesssize 128
	cull disable
	deformVertexes wave 100 sin 3 2 .1 0.1
	
        {
		map textures/liquids/protolava2.tga
                tcmod scale .2 .2
                tcmod scroll .04 .03
                tcMod turb 0 .1 0 .01
                blendFunc GL_ONE GL_ZERO
                rgbGen identity
	}
	{
		map textures/liquids/protolava.tga
                blendfunc blend
		tcMod turb 0 .2 0 .1
	}

}

textures/liquids/slime2
	{
	//	*************************************************		
	//	* SLIME Feb 11 1999 				*
	//	* IF YOU CHANGE THIS PLEASE COMMENT THE CHANGE	*
	//	*************************************************	

		// Added to g3map_global texture on May 11, 1999
		qer_editorimage textures/liquids/slime7e.tga
		q3map_lightimage textures/liquids/slime7e4.tga
		q3map_globaltexture
		qer_trans .5

		surfaceparm noimpact
		surfaceparm lava
		surfaceparm trans		

		q3map_surfacelight 2000
		tessSize 64
		cull disable
		deformVertexes wave 100 sin 2 4 0 .5
		
		
		{	
			map $lightmap
			rgbgen identity
		}
		

		{
			map textures/liquids/slime7e.tga
			blendFunc GL_DST_COLOR GL_SRC_ALPHA
			rgbGen identity
			alphaGen lightingSpecular
			tcMod turb 0 .2 .5 .05
			tcmod scroll .01 .01
		}


		
		{
			map textures/liquids/slime7e4.tga
			blendfunc add
			rgbgen wave inversesawtooth 0 1 0 4
			tcMod turb 0 .2 .5 .05
			tcmod scroll .01 .01
		}



		// 	END
	}

textures/liquids/watertest

//	************************************************************
//	*      Water test with environmentmap. Created September 21*
//	************************************************************
	
	{
		qer_editorimage textures/liquids/pool3d_3b.tga
		qer_trans .5
		q3map_globaltexture
		surfaceparm trans
		surfaceparm nonsolid
		surfaceparm water

		cull disable
		tessSize 32
		deformVertexes normal .1 .5

		{
			map textures/liquids/pool3d_3f.tga
			blendfunc add
			tcgen environment
			rgbgen wave sin .25 0 0 0
			tcmod scale 2 1
			tcMod scroll .05 .05
		}

		




	}


textures/liquids/flatripplewater_1500
{
	// Added to g3map_global texture on May 11, 1999
	qer_editorimage textures/liquids/pool2.tga
	q3map_globaltexture
	q3map_lightsubdivide 32
	//surfaceparm trans
	//surfaceparm nonsolid
	surfaceparm noimpact
	surfaceparm lava
	surfaceparm nolightmap
	q3map_surfacelight 500
	cull disable
	
//	tesssize 128
//	cull disable
	deformVertexes wave 100 sin 3 2 .1 0.1
	
	{
		map textures/liquids/pool2.tga
		tcMod turb 0 .2 0 .1
	}
}

textures/liquids/xctf_painwater
	{
		qer_editorimage textures/liquids/pool2.tga
		q3map_globaltexture
		qer_trans .5

		surfaceparm trans
		surfaceparm nonsolid
		surfaceparm water
		surfaceparm nolightmap
		q3map_surfacelight 450

		cull disable
		tesssize 64
		deformVertexes wave 100 sin 1 1 1 .1
		{
			map textures/liquids/pool2.tga
			blendfunc GL_ONE GL_SRC_COLOR	
			tcMod scale .03 .03
			tcMod scroll .001 .001
		}
		{
			map textures/liquids/pool3.tga
			blendfunc GL_DST_COLOR GL_ONE
			tcMod turb .1 .1 0 .01
			tcMod scale .5 .5
			tcMod scroll -.025 .02
		}
                {
			map textures/sfx/bolts.tga
			blendfunc GL_ONE GL_ONE
			tcMod scale .2 .2
                        //tcGen environment
			tcMod scroll .1 .1
                        tcmod rotate 290
                        //rgbgen wave sin .1 1 5 .1
		}
	}
textures/liquids/clear_calm1

//	*********************************************************
//	*      Medium clear calm water. Created September 9	*
//	*********************************************************
		
	{
		qer_editorimage textures/liquids/pool3d_3e.tga
		qer_trans .5
		q3map_globaltexture
		surfaceparm trans
		surfaceparm nonsolid
		surfaceparm water
	
		cull disable
		deformVertexes wave 64 sin .25 .25 0 .5	
		{ 
			map textures/liquids/pool3d_5e.tga
			blendFunc GL_dst_color GL_one
			rgbgen identity
			tcmod scale .5 .5
			tcmod scroll .025 .01
		}
	
		{ 
			map textures/liquids/pool3d_3e.tga
			blendFunc GL_dst_color GL_one
			tcmod scale -.5 -.5
			tcmod scroll .025 .025
		}

	
		{
			map $lightmap
			blendFunc GL_dst_color GL_zero
			rgbgen identity		
		}
	
	

//	END
	}

textures/liquids/clear_ripple3

//	*********************************************************
//	*      Medium clear ripple water. Created September 9	*
//	*********************************************************
		
	{
		qer_editorimage textures/liquids/pool3d_3c.tga
		qer_trans .5
		q3map_globaltexture
		surfaceparm trans
		surfaceparm nonsolid
		surfaceparm water
	
		cull disable
		deformVertexes wave 64 sin .5 .5 0 .5	
		
		{ 
			map textures/liquids/pool3d_5c.tga
			blendFunc GL_dst_color GL_one
			rgbgen identity
			tcmod scale .5 .5
			tcmod transform 1.5 0 1.5 1 1 2
			tcmod scroll -.05 .001
		}
	
		{ 
			map textures/liquids/pool3d_6c.tga
			blendFunc GL_dst_color GL_one
			rgbgen identity
			tcmod scale .5 .5
			tcmod transform 0 1.5 1 1.5 2 1
			tcmod scroll .025 -.001
		}

		{ 
			map textures/liquids/pool3d_3c.tga
			blendFunc GL_dst_color GL_one
			rgbgen identity
			tcmod scale .25 .5
			tcmod scroll .001 .025
		}
	
		{
			map $lightmap
			blendFunc GL_dst_color GL_zero
			rgbgen identity		
		}
	

//	END
	}


textures/liquids/clear_ripple2

//	*********************************************************
//	*      Small clear ripple water. Created September 9	*
//	*********************************************************
	
	{
		qer_editorimage textures/liquids/pool3d_3b.tga
		qer_trans .5
		q3map_globaltexture
		surfaceparm trans
		surfaceparm nonsolid
		surfaceparm water

		cull disable
		deformVertexes wave 64 sin .5 .5 0 .5	

		{ 
			map textures/liquids/pool3d_5b.tga
			blendFunc GL_dst_color GL_one
			rgbgen identity
			tcmod scale .5 .5
			tcmod transform 1.5 0 1.5 1 1 2
			tcmod scroll -.05 .001
		}
	
		{ 
			map textures/liquids/pool3d_6b.tga
			blendFunc GL_dst_color GL_one
			rgbgen identity
			tcmod scale .5 .5
			tcmod transform 0 1.5 1 1.5 2 1
			tcmod scroll .025 -.001
		}

		{ 
			map textures/liquids/pool3d_3b.tga
			blendFunc GL_dst_color GL_one
			rgbgen identity
			tcmod scale .25 .5
			tcmod scroll .001 .025
		}

		{
			map $lightmap
			blendFunc GL_dst_color GL_zero
			rgbgen identity		
		}
	

//	END
	}


textures/liquids/clear_ripple1

//	*********************************************************
//	*      Large clear ripple water. Created September 9	*
//	*********************************************************
	
	{
		qer_editorimage textures/liquids/pool3d_3.tga
		qer_trans .5
		q3map_globaltexture
		surfaceparm trans
		surfaceparm nonsolid
		surfaceparm water

		cull disable
		deformVertexes wave 64 sin .5 .5 0 .5	
	
		
		{ 
			map textures/liquids/pool3d_5.tga
			blendFunc GL_dst_color GL_one
			rgbgen identity
			tcmod scale .5 .5
			tcmod transform 1.5 0 1.5 1 1 2
			tcmod scroll -.05 .001
		}
	
		{ 
			map textures/liquids/pool3d_6.tga
			blendFunc GL_dst_color GL_one
			rgbgen identity
			tcmod scale .5 .5
			tcmod transform 0 1.5 1 1.5 2 1
			tcmod scroll .025 -.001
		}

		{ 
			map textures/liquids/pool3d_3.tga
			blendFunc GL_dst_color GL_one
			rgbgen identity
			tcmod scale .25 .5
			tcmod scroll .001 .025
		}	

		{
			map $lightmap
			blendFunc GL_dst_color GL_zero
			rgbgen identity		
		}
//	END
	}

textures/liquids/clear_ripple1_q3dm1

//	*********************************************************
//	*      Large clear ripple water. Created September 9	*
//	*********************************************************
	
	{
		qer_editorimage textures/liquids/pool3d_3.tga
		qer_trans .5
		q3map_globaltexture
		surfaceparm trans
		surfaceparm nonsolid
		surfaceparm water

		cull disable
		deformVertexes wave 64 sin .5 .5 0 .5	
	
		
		{ 
			map textures/liquids/pool3d_5.tga
			blendFunc GL_dst_color GL_one
			rgbgen identity
			tcmod scale .5 .5
			tcmod transform 1.5 0 1.5 1 1 2
			tcmod scroll -.05 .001
		}
	
		{ 
			map textures/liquids/pool3d_6.tga
			blendFunc GL_dst_color GL_one
			rgbgen identity
			tcmod scale .5 .5
			tcmod transform 0 1.5 1 1.5 2 1
			tcmod scroll .025 -.001
		}

		{ 
			map textures/liquids/pool3d_3.tga
			blendFunc GL_dst_color GL_one
			rgbgen identity
			tcmod scale .25 .5
			tcmod scroll .001 .025
		}	

		{
			map $lightmap
			blendFunc GL_dst_color GL_zero
			rgbgen identity		
		}
//	END
	}


textures/liquids/mercury
{
        tesssize 128
	deformVertexes wave 100 sin 5 5 0 0.4
        {
		rgbGen identity
		map $lightmap
                blendFunc GL_ONE GL_ONE
	}
	{
		map textures/liquids/mercury.tga
		blendFunc GL_DST_COLOR GL_SRC_ALPHA
		rgbGen identity
		alphaGen lightingSpecular
	}
        {
		map textures/base_trim/tinfx.tga
                tcgen environment
		blendFunc GL_ONE GL_ONE
		rgbGen identity
	}
       
        {
		map textures/liquids/mercury.tga
		//blendFunc GL_ONE GL_ONE
                blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}       
}
textures/liquids/proto_spec
{
       // tesssize 128
	cull disable
	//deformVertexes wave 100 sin 3 2 .1 2
         

	{
                map $lightmap
		//map textures/effects/envmapdim2.tga
                blendFunc GL_ONE GL_ONE
		tcgen environment
		 tcmod scale .25 .25
                //tcmod scroll -.05 -.5
		rgbgen identity      
	}
        // {
	//	map textures/liquids/proto_gruel2.tga
        //        tcmod scroll .01 .02
        //        tcmod scale .03  .04
	//	blendFunc GL_ONE GL_ONE
	//	rgbGen identity
	//}
        //{
	//	map textures/liquids/proto_spec.tga
	//        //blendFunc GL_DST_COLOR GL_SRC_ALPHA
        //        blendFunc GL_ONE_MINUS_SRC_ALPHA GL_SRC_ALPHA
        //        tcmod scroll -.05 -.07
	//	rgbGen identity
	//	alphaGen lightingSpecular
	//}
		
	
	{
		map textures/liquids/proto_spec.tga
                tcmod scroll .01 .02
		blendFunc GL_ONE_MINUS_SRC_ALPHA GL_SRC_ALPHA
		rgbGen identity
	}
        {
			map textures/liquids/proto_gruel2.tga
			blendfunc GL_ONE GL_ONE
			tcMod scale .2 .2
			tcMod scroll .01 .02
        }
       

	{
		map $lightmap
                //blendFunc GL_ONE GL_ONE
    		rgbgen identity
	        blendFunc GL_DST_COLOR GL_ZERO
               
	}

			
}

textures/liquids/lavahell
{
	// Added to g3map_global texture on May 11, 1999
	q3map_globaltexture
	surfaceparm trans
	//surfaceparm nonsolid
	surfaceparm noimpact
	surfaceparm lava
	surfaceparm nolightmap
	q3map_surfacelight 600
	cull disable
	
	tesssize 128
	cull disable
	deformVertexes wave 100 sin 3 2 .1 0.1
	
	{
		map textures/liquids/lavahell.tga
		tcMod turb 0 .2 0 .1
	}


	
//	END
}

textures/liquids/lavahell_xdm1
{
	qer_editorimage textures/liquids/lavahell.tga
	// Added to g3map_global texture on May 11, 1999
	q3map_globaltexture
	//surfaceparm trans
	//surfaceparm nonsolid
	surfaceparm nodlight
	surfaceparm noimpact
	//surfaceparm lava
	surfaceparm nolightmap
	q3map_surfacelight 600
	cull disable
	
	tesssize 128
	cull disable
	deformVertexes wave 100 sin 3 2 .1 0.1
	
	{
		map textures/liquids/lavahell.tga
		tcMod turb 0 .2 0 .1
	}
	
//	END
}

textures/liquids/lavahell_1000
{
	// Added to g3map_global texture on May 11, 1999
	qer_editorimage textures/liquids/lavahell.tga
	q3map_globaltexture
	surfaceparm trans
	//surfaceparm nonsolid
	surfaceparm noimpact
	surfaceparm lava
	surfaceparm nolightmap
	q3map_surfacelight 1000
	cull disable
	
	tesssize 128
	cull disable
	deformVertexes wave 100 sin 3 2 .1 0.1
	
	{
		map textures/liquids/lavahell.tga
		tcMod turb 0 .2 0 .1
	}
}

textures/liquids/lavahell_2000
{
	// Added to g3map_global texture on May 11, 1999
	qer_editorimage textures/liquids/lavahell.tga
	q3map_globaltexture
	surfaceparm trans
	//surfaceparm nonsolid
	surfaceparm noimpact
	surfaceparm lava
	surfaceparm nolightmap
	q3map_surfacelight 2000
	cull disable
	
	tesssize 128
	cull disable
	deformVertexes wave 100 sin 3 2 .1 0.1
	
	{
		map textures/liquids/lavahell.tga
		tcMod turb 0 .2 0 .1
	}
}
textures/liquids/lavahell_750
{
	// Added to g3map_global texture on May 11, 1999
	qer_editorimage textures/liquids/lavahell.tga
	q3map_globaltexture
	q3map_lightsubdivide 32
	surfaceparm trans
	//surfaceparm nonsolid
	surfaceparm noimpact
	surfaceparm lava
	surfaceparm nolightmap
	q3map_surfacelight 500
	cull disable
	
	tesssize 128
	cull disable
	deformVertexes wave 100 sin 3 2 .1 0.1
	
	{
		map textures/liquids/lavahell.tga
		tcMod turb 0 .2 0 .1
	}
}

textures/liquids/flatlavahell_1500
{
	// Added to g3map_global texture on May 11, 1999
	qer_editorimage textures/liquids/lavahell.tga
	q3map_globaltexture
	q3map_lightsubdivide 32
	//surfaceparm trans
	//surfaceparm nonsolid
	surfaceparm noimpact
	surfaceparm lava
	surfaceparm nolightmap
	q3map_surfacelight 1500
	cull disable
	
//	tesssize 128
//	cull disable
	deformVertexes wave 100 sin 3 2 .1 0.1
	
	{
		map textures/liquids/lavahell.tga
		tcMod turb 0 .2 0 .1
	}
       
}


textures/liquids/lavahell_2500
{
	// Added to g3map_global texture on May 11, 1999
	qer_editorimage textures/liquids/lavahell.tga
	q3map_globaltexture
	surfaceparm trans
	//surfaceparm nonsolid
	surfaceparm noimpact
	surfaceparm lava
	surfaceparm nolightmap
	q3map_surfacelight 2500
	cull disable
	
	tesssize 128
	cull disable
	deformVertexes wave 100 sin 3 2 .1 0.1
	
	{
		map textures/liquids/lavahell.tga
		tcMod turb 0 .2 0 .1
	}
}

textures/liquids/lavahelldark
{
	// Added to g3map_global texture on May 11, 1999
	qer_editorimage textures/liquids/lavahell.tga
	q3map_globaltexture
	surfaceparm trans
	//surfaceparm nonsolid
	surfaceparm noimpact
	surfaceparm lava
	surfaceparm nolightmap
	q3map_surfacelight 150
	cull disable
	
	tesssize 128
	cull disable
	deformVertexes wave 100 sin 3 2 .1 0.1
	
	{
		map textures/liquids/lavahell.tga
		tcMod turb 0 .2 0 .1
	}
	
//	END
}

textures/liquids/lavahellflat_400
{
	// Added to g3map_global texture on May 11, 1999
	qer_editorimage textures/liquids/lavahell.tga
	q3map_globaltexture
	surfaceparm trans
	//surfaceparm nonsolid
	surfaceparm noimpact
	surfaceparm lava
	surfaceparm nolightmap
	q3map_surfacelight 400
	cull disable
	
	tesssize 128
	cull disable
	//deformVertexes wave 100 sin 3 2 .1 0.1
	
	{
		map textures/liquids/lavahell.tga
		tcMod turb 0 .2 0 .1
	}
	
//	END
}

textures/liquids/lavahell2
{
	// Added to g3map_global texture on May 11, 1999
	qer_editorimage textures/liquids/lavahell2.tga
	q3map_globaltexture
	qer_nocarve
	surfaceparm noimpact
	surfaceparm lava
	surfaceparm nolightmap
	q3map_surfacelight 3000
	tesssize 256
	cull disable
	deformVertexes wave 100 sin 5 5 .5 0.02
	fogparms ( 0.8519142 0.309723 0.0 ) 128

	{
		map textures/liquids/lavahell.tga
		tcMod turb .25 0.2 1 0.02
		tcMod scroll 0.1 0.1
	}
}

textures/liquids/calm_pool
	{
		//	*************************************************
		//	* CALM GREEN WATER POOL Feb 19, 1998		*
		//	* IF YOU CHANGE THIS PLEASE COMMENT THE CHANGE	*
		//	*************************************************

		// Added to g3map_global texture on May 11, 1999
		qer_editorimage textures/liquids/pool2.tga
		qer_trans .5
		q3map_globaltexture

		surfaceparm trans
		surfaceparm nonsolid
		surfaceparm water
		surfaceparm nolightmap

		cull disable
		tesssize 64
		deformVertexes wave 100 sin 1 1 1 .1
		
	
	
		{
			map textures/liquids/pool2.tga
			blendfunc GL_ONE GL_SRC_COLOR	
			tcMod scale .03 .03
			tcMod scroll .001 .001
		}
	
		{
			map textures/liquids/pool3.tga
			blendfunc GL_DST_COLOR GL_ONE
			tcMod turb .1 .1 0 .01
			tcMod scale .5 .5
			tcMod scroll -.025 .02
		}


		//	END
	}

textures/liquids/ps_bloodwater1
	{
		//	*************************************************
		//	*  I made this for ps_dm5 on June 13 1999	*
		//      *      (it's okay if you fuck it up)		*
		//	*************************************************

		qer_editorimage textures/liquids/ps_bloodwater1.tga
		q3map_globaltexture
		qer_trans .5

		surfaceparm trans
		surfaceparm nonsolid
		surfaceparm water
		surfaceparm nolightmap
		q3map_surfacelight 50

		cull disable
		tesssize 64
		deformVertexes wave 100 sin 1 1 1 .1
		
	
	
		{
			map textures/liquids/ps_bloodwater1.tga
			blendfunc GL_ONE GL_SRC_COLOR	
			tcMod scale .03 .03
			tcMod scroll .001 .001
		}
	
		{
			map textures/liquids/ps_bloodwater1.tga
			blendfunc GL_DST_COLOR GL_ONE
			tcMod turb .1 .1 0 .01
			tcMod scale .5 .5
			tcMod scroll -.025 .02
		}


		//	END
	}


textures/liquids/calm_poollight
	{
		//	*************************************************
		//	* CALM GREEN WATER POOL may 5 1999
		//    * same as calm water but emits light		*
		//	* IF YOU CHANGE THIS PLEASE COMMENT THE CHANGE	*
		//	*************************************************

	
		qer_editorimage textures/liquids/pool3d_4b.tga
		q3map_globaltexture
		qer_trans .75

		surfaceparm trans
		surfaceparm nonsolid
		surfaceparm water
		surfaceparm nolightmap
		q3map_surfacelight 50

		cull disable
		tesssize 64
		deformVertexes wave 100 sin 1 1 1 .1
		
	
	
		{ 
			map textures/liquids/pool3d_5c2.tga
			blendFunc GL_dst_color GL_zero
			rgbgen identity
			tcmod scale .5 .5
			tcmod transform 1.5 0 1.5 1 1 2
			tcmod scroll -.05 .001
		}
	
		{ 
			map textures/liquids/pool3d_6c2.tga
			blendFunc GL_dst_color GL_zero
			rgbgen identity
			tcmod scale .5 .5
			tcmod transform 0 1.5 1 1.5 2 1
			tcmod scroll .025 -.001
		}

		{ 
			map textures/liquids/pool3d_3c2.tga
			blendFunc GL_dst_color GL_zero
			rgbgen identity
			tcmod scale .25 .5
			tcmod scroll .001 .025
		}
		{
			map textures/liquids/pool3d_4b2.tga
			blendfunc add
			tcmod scale .125 .125	
			rgbgen identity
		}


		//	END
	}


textures/liquids/calm_pool2
	{
		//	*************************************************
		//	* CALM GREEN WATER POOL Feb 19, 1998		*
		//	* IF YOU CHANGE THIS PLEASE COMMENT THE CHANGE	*
		//	*************************************************

		// Added to g3map_global texture on May 11, 1999
		qer_editorimage textures/liquids/pool3.tga
		q3map_globaltexture
		qer_trans .5

		surfaceparm trans
		surfaceparm nonsolid
		surfaceparm water

		cull disable
		tesssize 64
		deformVertexes wave 100 sin 1 1 1 .1
		
	
	
			

		{
			map textures/liquids/pool2.tga
			blendfunc GL_one GL_src_color
			tcMod scale .03 .03
			tcMod scroll .001 .001
		}
	
		{
			map textures/liquids/pool3.tga 
			blendfunc GL_DST_COLOR GL_one
			tcMod scale .5 .5
			tcMod scroll -.025 .02
		}

		{
			map $lightmap
			blendfunc GL_dst_color GL_one
			tcmod turb 0 .001 0 1
		}	

		{
			map textures/liquids/pj_e3_reflection.tga
			blendfunc gl_dst_color gl_one
			tcgen environment
		}

			



		//	END
	}


textures/liquids/pool
	{
		//	*************************************************
		//	* GREEN WATER POOL Feb 19, 1998			*
		//	* IF YOU CHANGE THIS PLEASE COMMENT THE CHANGE	*
		//	*************************************************

		// Added to g3map_global texture on May 11, 1999
		qer_trans .5

		surfaceparm trans
		surfaceparm nonsolid
		surfaceparm water
		surfaceparm nolightmap

		q3map_globaltexture		
		// so the scMod scales work properly 5/11/99

		cull disable
		tesssize 64
		deformVertexes wave 100 sin 1 1 1 .1
		
	
	
		{
			map textures/liquids/pool2.tga
			blendfunc GL_ONE GL_SRC_COLOR	
			tcMod scale .03 .03
			tcMod scroll .001 .001
		}
	
		{
			map textures/liquids/pool.tga
			blendfunc GL_DST_COLOR GL_ONE
			tcMod scale .2 .2
			tcMod stretch sin 1 .1 0 .05
			rgbGen wave sin .5 .5 0 .05
			tcMod scroll -.001 .001
		}
	
		{
			map textures/liquids/pool3.tga
			blendfunc GL_DST_COLOR GL_ONE
			tcMod turb .1 .1 0 .01
			tcMod scale .5 .5
			tcMod scroll -.025 .02
		}
	
		{
			map textures/liquids/pool4.tga
			blendfunc GL_DST_COLOR GL_ONE
			tcMod scale .3 .3
			tcMod turb .1 .01 0 .01
			tcMod stretch sin 1 .3 0 .015
			rgbGen wave sin .5 .5 0 .07
			tcMod scroll .001 -.001
		}

		//	END
	}
	
textures/liquids/calm_pool_bgtess
	{
		//	*************************************************
		//	* CALM GREEN WATER POOL Feb 19, 1998		*
		//	* IF YOU CHANGE THIS PLEASE COMMENT THE CHANGE	*
		//      * Large tesselations added on May 26, '99
		//	*************************************************

		// Added to g3map_global texture on May 11, 1999
		qer_editorimage textures/liquids/pool2.tga
		qer_trans .5
		q3map_globaltexture

		surfaceparm trans
		surfaceparm nonsolid
		surfaceparm water
		surfaceparm nolightmap

		cull disable
		tesssize 256
		deformVertexes wave 256 sin 1 1 1 .1
		
	
	
		{
			map textures/liquids/pool2.tga
			blendfunc GL_ONE GL_SRC_COLOR	
			tcMod scale .03 .03
			tcMod scroll .001 .001
		}
	
		{
			map textures/liquids/pool3.tga
			blendfunc GL_DST_COLOR GL_ONE
			tcMod turb .1 .1 0 .01
			tcMod scale .5 .5
			tcMod scroll -.025 .02
		}


		//	END
	}

textures/liquids/ripplewater2
	{
		//	*************************************************
		//	* Ripply Water Feb 16 1999			*
		//	* IF YOU CHANGE THIS PLEASE COMMENT THE CHANGE	*
		//	*************************************************

		// Added to g3map_global texture on May 11, 1999
		q3map_globaltexture
		qer_trans .5
	
		surfaceparm trans
		surfaceparm nonsolid
		surfaceparm water
		surfaceparm nolightmap

		// JDC: testing foggy water
		surfaceparm	fog
		fogparms ( .5 .12 1 ) 200

		tesssize 64
		deformVertexes wave 100 triangle 2 2 1 0.1
		
		{
			map textures/liquids/ripplewater3.tga
			blendfunc GL_ONE GL_SRC_COLOR	
			tcMod scale .05 .05
			tcGen environment
			tcMod scroll .001 .001
		}
	
		{
			map textures/liquids/ripplewater2.tga
			blendfunc GL_DST_COLOR GL_ONE
			tcMod turb .1 .1 0 .01
			tcMod scale .5 .5
			tcMod scroll -.025 .02
		}

		{
			map textures/liquids/ripplewater.tga
			blendFunc GL_DST_COLOR GL_ONE
			tcMod turb .1 .07 .1 .01
			tcMod scale .5 .5
			tcMod stretch sin 1 .1 0 .01
			tcMod scroll 0 .025
		}	

		

		//	END
	}

textures/liquids/proto_pool3
	{
		///////////////////////////////////////////
		//   Water Fuckage June 18 99 Kenneth	 //
		///////////////////////////////////////////
                
		q3map_globaltexture
	    	qer_trans .80
                surfaceparm trans
		surfaceparm nonsolid
		surfaceparm water
                q3map_surfacelight 50
		cull disable
                // tesssize 256
	        //deformVertexes wave 100 sin 1 5 .1 .5
	        
		
		{
			map textures/liquids/proto_pool3.tga
			blendfunc GL_ONE GL_ONE //SRC_COLOR
			tcMod scale 1 1
			tcMod scroll .02 .02
                       
		}
	        {
			map textures/liquids/proto_poolpass2.tga
			blendfunc GL_ONE GL_ONE
			tcMod scale .5 .6
			tcMod scroll .06 .04
		}
	        {
			map textures/liquids/proto_poolpass.tga
			blendfunc GL_ONE GL_ONE
			tcMod scale .2 .2
			tcMod scroll .05 .05
		}
	        {
		        map $lightmap
			rgbgen identity
			blendfunc GL_DST_COLOR GL_ZERO
                }
               
		

		//	END
	}
 textures/liquids/proto_poolpass
        {
		///////////////////////////////////////////
	        //Electric Slime. FOUR passes of FURY ---//
		///////////////////////////////////////////
                
		q3map_globaltexture
	    	//qer_editorimage textures/liquids/proto_pool3.tga
                 q3map_lightimage textures/liquids/proto_pool3.tga
	        q3map_globaltexture	
	        q3map_surfacelight 240
		// tesssize 256
	        //deformVertexes wave 100 sin 1 5 .1 .5
	        surfaceparm slime
		
		{
	        	map textures/liquids/proto_gruel2.tga
		        //tcMod scale 1 1
                         tcMod scale .5 .6
			//tcMod scroll .4 .01
                        tcMod turb .1 .25 0 -.1
                       
		}
	       //{
		//	map textures/liquids/proto_poolpass2.tga
		//	blendfunc GL_ONE GL_ONE
		//	tcMod scale .5 .6
		//	//tcMod scroll .6 .04
                //        tcMod turb .1 .25 0 .01
		//}
	        {
			map textures/liquids/proto_poolpass.tga
			blendfunc GL_ONE GL_ONE
			tcMod scale .2 .2
			//tcMod scroll .5 .05
                        tcMod turb .1 .25 0 .07
		}
	        {
		        map $lightmap
			rgbgen identity
			blendfunc GL_DST_COLOR GL_ZERO
                }
                {
			map textures/sfx/bolts.tga
			blendfunc GL_ONE GL_ONE
			tcMod scale .2 .2
                        //tcGen environment
			tcMod scroll .1 .1
                        tcmod rotate 290
                        //rgbgen wave sin .1 1 5 .1
		}
		

		//	END
	}
textures/liquids/jello
        {
		///////////////////////////////////////////
	        //DANGER JELLO! FOUR passes of FURY ---  //
		///////////////////////////////////////////
                
		q3map_globaltexture
	    	//qer_editorimage textures/liquids/proto_pool3.tga
                 q3map_lightimage textures/liquids/proto_pool3.tga
	        q3map_globaltexture	
	        q3map_surfacelight 240
		// tesssize 256
	        //deformVertexes wave 100 sin 1 5 .1 .5
	        surfaceparm slime
		
		{
	        	map textures/liquids/jello.tga
		        //tcMod scale 1 1
                         tcMod scale .5 .6
			//tcMod scroll .4 .01
                        tcMod turb .1 .25 0 -.1
                       
		}
	        {
			map textures/liquids/jello2.tga
			blendfunc GL_ONE GL_ONE
			tcMod scale .2 .2
			//tcMod scroll .5 .05
                        tcMod turb .1 .25 0 .07
		}
	        {
		        map $lightmap
			rgbgen identity
			blendfunc GL_DST_COLOR GL_ZERO
                }
                {
			map textures/sfx/bolts2.tga
			blendfunc GL_ONE GL_ONE
			tcMod scale .2 .2
                        //tcGen environment
			tcMod scroll 5  5
                        tcmod rotate 296
                        //rgbgen wave sin .1 1 5 .1
		}
		

		//	END
	}
 textures/liquids/proto_slime
        {
		///////////////////////////////////////////
		//   Slimage June 22 99 Kenneth	         //
		///////////////////////////////////////////
                
		q3map_globaltexture
	     // qer_trans .001
                surfaceparm trans
		surfaceparm nonsolid
		surfaceparm water
                cull disable
	        
		
		{
			map textures/liquids/proto_slime.tga
		        alphaFunc GE128
		        depthWrite
		        //rgbGen vertex
			tcMod scale 1 1
			tcMod scroll .02 .02
                       
		}
	        {
		        map $lightmap
			rgbgen identity
			blendfunc GL_DST_COLOR GL_ZERO
                }
		

		//	END
	}
textures/liquids/proto_gruel
        {
		///////////////////////////////////////////////////
		//   Wrongful, Afterbirth-esque goo June 24 99 //
		///////////////////////////////////////////////////
              q3map_globaltexture
	    	qer_trans 80
                surfaceparm trans
		surfaceparm nonsolid
                surfaceparm nomarks
		surfaceparm water
		cull disable
                //tesssize 256
	        //deformVertexes wave 100 sin 1 5 .1 .5
	        
		
		{
			map textures/liquids/proto_gruel.tga
			// blendfunc GL_ONE GL_ONE
			tcMod scale 1 1
			tcMod scroll .02 .02
                       
		}
	        {
			map textures/liquids/proto_gruel.tga
			blendfunc GL_ONE GL_ONE
			tcMod scale .5 .6
			tcMod scroll .06 .04
		}
	        //{
		//	map textures/liquids/proto_poolpass.tga
		//	blendfunc GL_ONE GL_ONE
		//	tcMod scale .2 .2
		//	tcMod scroll .05 .05
		//}
	        {
		        map $lightmap
			rgbgen identity
			blendfunc GL_DST_COLOR GL_ZERO
                }
		
        }
textures/liquids/xripplewater2
	{
		//	*************************************************
		//	* Ripply Water Feb 16 1999			*
		//	* IF YOU CHANGE THIS PLEASE COMMENT THE CHANGE	*
		//	*************************************************
		// June.9.1999 -- Added q3map_surfacelight -- Xian
		// Added to g3map_global texture on May 11, 1999
		qer_editorimage textures/liquids/ripplewater2.tga
		q3map_surfacelight 600
		q3map_globaltexture
		qer_trans .5
	
		surfaceparm trans
		surfaceparm nonsolid
		surfaceparm water
		surfaceparm nolightmap

		// JDC, testing backshader cull disable
		tesssize 64
		deformVertexes wave 100 triangle 2 2 1 0.1
		
		surfaceparm fog
		fogparms ( 0.3 0.3 1.0 ) 512

		{
			map textures/liquids/ripplewater3.tga
			blendfunc GL_ONE GL_SRC_COLOR	
			tcMod scale .05 .05
			tcGen environment
			tcMod scroll .001 .001
		}
	
		{
			map textures/liquids/ripplewater2.tga
			blendfunc GL_DST_COLOR GL_ONE
			tcMod turb .1 .1 0 .01
			tcMod scale .5 .5
			tcMod scroll -.025 .02
		}

		{
			map textures/liquids/ripplewater.tga
			blendFunc GL_DST_COLOR GL_ONE
			tcMod turb .1 .07 .1 .01
			tcMod scale .5 .5
			tcMod stretch sin 1 .1 0 .01
			tcMod scroll 0 .025
		}	

		

		//	END
	}

ripplewater2_back
	{
		// Added to g3map_global texture on May 11, 1999
		q3map_globaltexture
	//	this is used when looking from inside the water to the outside
		surfaceparm trans
		surfaceparm nonsolid
		surfaceparm water
		surfaceparm nolightmap

//		fogonly
		{
			map textures/liquids/ripplewater3.tga
			blendfunc GL_DST_COLOR GL_SRC_COLOR
			tcMod scale .05 .05
			tcGen environment
			tcMod scroll .001 .001
		}

		//	END
	}

textures/liquids/ripplewater2a
	{
		//	*************************************************
		//	* Ripply Water large tesselations MAr 15 1999			*
		//	* IF YOU CHANGE THIS PLEASE COMMENT THE CHANGE	*
		//	*************************************************

		// Added to g3map_global texture on May 11, 1999
		q3map_globaltexture
		qer_trans .5
		qer_editorimage textures/liquids/ripplewater2
		surfaceparm trans
		surfaceparm nonsolid
		surfaceparm water
		surfaceparm nolightmap

		cull disable
		tesssize 128
		deformVertexes wave 100 triangle 2 2 1 0.1
		

		{
			map textures/liquids/ripplewater3.tga
			blendfunc GL_ONE GL_SRC_COLOR	
			tcMod scale .05 .05
			tcGen environment
			tcMod scroll .001 .001
		}
	
		{
			map textures/liquids/ripplewater2.tga
			blendfunc GL_DST_COLOR GL_ONE
			tcMod turb .1 .1 0 .01
			tcMod scale .5 .5
			tcMod scroll -.025 .02
		}

		{
			map textures/liquids/ripplewater.tga
			blendFunc GL_DST_COLOR GL_ONE
			tcMod turb .1 .07 .1 .01
			tcMod scale .5 .5
			tcMod stretch sin 1 .1 0 .01
			tcMod scroll 0 .025
		}	
		//	END
	}

textures/liquids/slime1
	{
	//	*************************************************		
	//	* SLIME Feb 11 1999 				*
	//	* IF YOU CHANGE THIS PLEASE COMMENT THE CHANGE	*
	//	*************************************************	

		// Added to g3map_global texture on May 11, 1999
		qer_editorimage textures/liquids/slime7.tga
		q3map_lightimage textures/liquids/slime7.tga
		q3map_globaltexture
		qer_trans .5

		surfaceparm noimpact
		surfaceparm slime
		surfaceparm nolightmap
		surfaceparm trans		

		q3map_surfacelight 100
		tessSize 32
		cull disable

		deformVertexes wave 100 sin 0 1 .5 .5

		{
			map textures/liquids/slime7c.tga
			tcMod turb .3 .2 1 .05
			tcMod scroll .01 .01
		}
	
		{
			map textures/liquids/slime7.tga
			blendfunc GL_ONE GL_ONE
			tcMod turb .2 .1 1 .05
			tcMod scale .5 .5
			tcMod scroll .01 .01
		}

		{
			map textures/liquids/bubbles.tga
			blendfunc GL_ZERO GL_SRC_COLOR
			tcMod turb .2 .1 .1 .2
			tcMod scale .05 .05
			tcMod scroll .001 .001
		}		

		// 	END
	}

textures/liquids/slime1_2000
	{
	//	*************************************************		
	//	* SLIME Feb 11 1999 				*
	//	* IF YOU CHANGE THIS PLEASE COMMENT THE CHANGE	*
	//	*************************************************	

		// Added to g3map_global texture on May 11, 1999
		qer_editorimage textures/liquids/slime7.tga
		q3map_lightimage textures/liquids/slime7.tga
		q3map_globaltexture
		qer_trans .5

		surfaceparm noimpact
		surfaceparm slime
		surfaceparm nolightmap
		surfaceparm trans		

		q3map_surfacelight 2000
		tessSize 32
		cull disable

		deformVertexes wave 100 sin 0 1 .5 .5

		{
			map textures/liquids/slime7c.tga
			tcMod turb .3 .2 1 .05
			tcMod scroll .01 .01
		}
	
		{
			map textures/liquids/slime7.tga
			blendfunc GL_ONE GL_ONE
			tcMod turb .2 .1 1 .05
			tcMod scale .5 .5
			tcMod scroll .01 .01
		}

		{
			map textures/liquids/bubbles.tga
			blendfunc GL_ZERO GL_SRC_COLOR
			tcMod turb .2 .1 .1 .2
			tcMod scale .05 .05
			tcMod scroll .001 .001
		}		

		// 	END
	}



textures/liquids/lavacrust
	{
	//	*************************************************
	//	* CRUSTY LAVA Feb 11 1999			*
	//	* IF YOU CHANGE THIS PLEASE COMMENT THE CHANGE	*
	//	*************************************************
	// change 5/3 - lighting changed from 300 to 50	
	// change 5/5 - made lavacrust solid
	// Added to g3map_global texture on May 11, 1999
		q3map_globaltexture
		surfaceparm trans
		//surfaceparm lava

		
		q3map_surfacelight 50
		q3map_lightimage textures/liquids/lava.tga

		//tesssize 64
		//deformVertexes wave 100 sin 0 5 0 .05
		//cull disable

		{
			map textures/liquids/lava.tga
			tcMod turb 0 0.1 0 0.02
			rgbGen wave triangle .9 .1 0 1
			tcMod scroll -.02 -.02
		}

		{
			map textures/liquids/lavacrust.tga
			blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
			tcmod turb 0 .5 0 .005
			rgbgen identity
		}

		{
			map $lightmap
			blendfunc gl_dst_color gl_zero
			rgbgen identity
		}

			
		//	END
	}

textures/liquids/bloodwater_pj
	{
	//	*************************************************
	//	* Brighter BLOODY WATER Apr 7 1999		        *
	//	* IF YOU CHANGE THIS PLEASE COMMENT THE CHANGE	*
	//	*************************************************
	// Added to g3map_global texture on May 11, 1999
	qer_editorimage textures/liquids/bloodwater.tga
	q3map_globaltexture
	qer_trans .5
	
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm water
	surfaceparm nolightmap
	//testing out a dim surface light, march 19 - bj
	q3map_surfacelight 500
	tesssize 128
	cull disable
	deformVertexes wave 128 sin 1 1 .5 .5
	
	
		{
			map textures/liquids/bloodwater1.tga
			blendFunc GL_ONE GL_ONE
			tcGen environment
			tcMod scroll 0 .01
		}

		{
			map textures/liquids/bloodwater2.tga
			blendFunc GL_DST_COLOR GL_SRC_COLOR
			tcMod scale .1 .1
			tcMod turb .1 .1 1 .01
			tcMod scroll 0 .01
		}

		{
			map textures/liquids/bloodwater.tga
			blendFunc GL_DST_COLOR GL_SRC_COLOR
			tcMod scale .25 .25
			tcMod scroll 0 .007
		}
	
		//	END
	}

textures/liquids/bloodwater
	{
	//	*************************************************
	//	* BLOODY WATER Feb 16 1999		        *
	//	* IF YOU CHANGE THIS PLEASE COMMENT THE CHANGE	*
	//	*************************************************

	// Added to g3map_global texture on May 11, 1999
	q3map_globaltexture
	qer_trans .5

	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm water
	surfaceparm nolightmap
	//testing out a dim surface light, march 19 - bj
	q3map_surfacelight 200
	tesssize 64
	cull disable
	deformVertexes wave 100 sin 1 1 .5 .5
	
	
		{
			map textures/liquids/bloodwater1.tga
			blendFunc GL_ONE GL_ONE
			tcGen environment
			tcMod scroll 0 .01
		}

		{
			map textures/liquids/bloodwater2.tga
			blendFunc GL_DST_COLOR GL_SRC_COLOR
			tcMod scale .1 .1
			tcMod turb .1 .1 1 .01
			tcMod scroll 0 .01
		}

		{
			map textures/liquids/bloodwater.tga
			blendFunc GL_DST_COLOR GL_SRC_COLOR
			tcMod scale .25 .25
			tcMod scroll 0 .007
		}
	
		//	END
	}


textures/hell/lavasolid
{
	// Added to g3map_global texture on May 11, 1999
	q3map_globaltexture
	surfaceparm noimpact
	surfaceparm nolightmap
	q3map_surfacelight 450

	tesssize 64
	deformVertexes wave 100 sin 2 2 0 0.1

	{
		map textures/hell/lava2d.tga
		tcGen environment
		tcMod turb 0 0.25 0 0.05
	}

	{
		map textures/hell/lava2d.tga
		blendfunc GL_ONE GL_ONE
		tcMod turb 0 0.25 0 0.05
	}
}
