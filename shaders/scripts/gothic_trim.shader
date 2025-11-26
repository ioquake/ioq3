textures/gothic_trim/x_noblight
{   
   q3map_lightimage textures/gothic_trim/x_noblightfx.tga
   q3map_surfacelight 100
        {
                map textures/sfx/firegorre2.tga       
                tcmod scroll .1 1
                tcmod scale 1 1
	}   
        {
		map textures/gothic_trim/x_noblight.tga
                blendFunc blend
		rgbGen identity
	} 
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
        {
		map textures/gothic_trim/x_noblightfx.tga
                blendFunc add
		rgbGen wave sin .5 .5 0 .1
	} 
}
textures/gothic_trim/zinc_shiny
{   
    
        {
                map textures/effects/tinfx.tga       
                tcGen environment
                rgbGen identity
	}   
        {
		map textures/gothic_trim/zinc_shiny.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	} 
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
}
textures/gothic_trim/chrome_rivet
{   
    
        {
                map textures/effects/tinfx.tga       
                tcGen environment
                rgbGen identity
	}   
        {
		map textures/gothic_trim/chrome_rivet.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	} 
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
}
textures/gothic_trim/km_arena1tower4fx
{   
    
        {
                map textures/effects/tinfx.tga       
                tcGen environment
                rgbGen identity
	}   
        {
		map textures/gothic_trim/km_arena1tower4fx.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	} 
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
}
textures/gothic_trim/km_arena1towerblue
{   
    
        {
                map textures/effects/tinfx.tga       
                tcGen environment
                rgbGen identity
	}   
        {
		map textures/gothic_trim/km_arena1towerblue.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	} 
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
}         
//textures/gothic_trim/metalbase09_b_blocks15
//{              
//	{
//		map $lightmap
//		rgbGen identity
//	}
//
//
//       {
//		map textures/gothic_trim/metalbase09_b_blocks15.tga
//                blendFunc GL_dst_color GL_SRC_ALPHA
//		alphagen lightingspecular
//		rgbGen identity
//	}
//
//}
//

textures/gothic_trim/column2c_trans
{
	qer_editorimage textures/gothic_trim/column2c_test.tga
	surfaceparm nonsolid
	{
		map $lightmap
		rgbGen identity
	
	}
	{
		map textures/gothic_trim/column2c_test.tga
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO

	
	}
}

textures/gothic_trim/metalsupport4b_trans
{
	qer_editorimage textures/gothic_trim/metalsupport4b.tga
	surfaceparm nonsolid
	{
		map $lightmap
		rgbGen identity
	
	}
	{
		map textures/gothic_trim/metalsupport4b.tga
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO

	
	}
}

textures/gothic_trim/metalsupport4j_trans
{
	qer_editorimage textures/gothic_trim/metalsupport4j.tga
	surfaceparm nonsolid
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/gothic_trim/metalsupport4j.tga
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/gothic_trim/border6_trans
{
	qer_editorimage textures/gothic_trim/border6.tga
	surfaceparm nonsolid
	{
		map $lightmap
		rgbGen identity

	}
	{
		map textures/gothic_trim/border6.tga
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO

	}
}

textures/gothic_trim/pitted_rust2_trans
{
	qer_editorimage textures/gothic_trim/pitted_rust2.tga
	surfaceparm nonsolid
	{
		map $lightmap
		rgbGen identity
	
	}
	{
		map textures/gothic_trim/pitted_rust2.tga
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO

	
	}
}
textures/gothic_trim/pitted_rust3_trans
{
	qer_editorimage textures/gothic_trim/pitted_rust3.tga
	surfaceparm nonsolid
	{
		map $lightmap
		rgbGen identity
	
	}
	{
		map textures/gothic_trim/pitted_rust3.tga
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO

	
	}
}

textures/gothic_block/killtrim_trans
{
	qer_editorimage textures/gothic_block/killtrim.tga
	surfaceparm nonsolid
	{
		map $lightmap
		rgbGen identity
	
	}
	{
		map textures/gothic_block/killtrim.tga
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO

	
	}
}

textures/gothic_trim/baseboard09_p_shiny
{
	

	{
		map $lightmap
	//	map textures/base_wall/chrome_env.tga
	        rgbGen identity
	//	tcGen environment
	//	tcmod scale .25 .25
	}
	
	{
		map textures/gothic_trim/baseboard09_p_shiny.tga
		blendFunc GL_DST_COLOR GL_SRC_ALPHA
		alphagen lightingspecular
	//	blendFunc GL_ONE_MINUS_SRC_ALPHA GL_SRC_ALPHA	
		rgbGen identity
	}
	//{
	//	map $lightmap
	//	blendfunc gl_dst_color gl_zero
	//	rgbGen identity
	//}
}


textures/gothic_trim/km_arena1tower4_shiny
{

	{
		map $lightmap
		rgbgen identity      
	}
	
	{
		map textures/gothic_trim/km_arena1tower4_shiny.tga
		blendFunc GL_DST_COLOR GL_SRC_ALPHA
		rgbGen identity
		alphaGen lightingSpecular
	}

			
}



textures/gothic_trim/metalsupport4h_shiny
{

	{
		map $lightmap
		rgbgen identity      
	}
	
	{
		map textures/gothic_trim/metalsupport4h_shiny.tga
		blendFunc GL_DST_COLOR GL_SRC_ALPHA
		rgbGen identity
		alphaGen lightingSpecular
	}

			
}

textures/gothic_trim/metalsupport4h_trans
{
	qer_editorimage textures/gothic_trim/metalsupport4h_shiny
	surfaceparm nonsolid
	{
		map $lightmap
		rgbgen identity      
	}
	
	{
		map textures/gothic_trim/metalsupport4h_shiny.tga
		blendFunc GL_DST_COLOR GL_SRC_ALPHA
		rgbGen identity
		alphaGen lightingSpecular
	}

			
}

textures/gothic_trim/supportborderside_shiney
{
	qer_editorimage textures/gothic_trim/xsupportborderside_shiney.tga
	{
		map $lightmap
		rgbgen identity      
	}
	
	{
		map textures/gothic_trim/xsupportborderside_shiney.tga
		blendFunc GL_DST_COLOR GL_SRC_ALPHA
		rgbGen identity
		alphaGen lightingSpecular
	}

			
}

textures/gothic_trim/metlsupport4i_shiney
{
	qer_editorimage textures/gothic_trim/xmetalsupport4i_alpha.tga
	{
		map $lightmap
		rgbgen identity      
	}
	
	{
		map textures/gothic_trim/xmetalsupport4i_alpha.tga
		blendFunc GL_DST_COLOR GL_SRC_ALPHA
		rgbGen identity
		alphaGen lightingSpecular
	}

			
}

textures/gothic_trim/barbedwire
{	
        surfaceparm trans		
	cull disable

	// RUSTY Barbed wire
	{
		map textures/gothic_trim/barbedwire.tga
		// tcMod scale 2 2
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
textures/gothic_button/timbutton
	{
	q3map_lightimage textures/gothic_button/timbutton2.tga
	q3map_surfacelight 10
	//light 1

		{
			map textures/gothic_button/timbutton.tga
		}


		{
			map $lightmap
			blendfunc gl_dst_color gl_zero
			rgbgen identity
		}

		{
			map textures/gothic_button/timbutton2.tga
			blendfunc gl_one gl_one
			rgbgen wave sin .5 .5 0 1
		}

	}

		

textures/gothic_wall/window_evil6a

{
	q3map_lightimage textures/gothic_wall/window_evil6a_glow.tga
	q3map_surfacelight 50
	//light 1

	{
		map $lightmap
		rgbGen identity
	}
	
	{
		map textures/gothic_wall/window_evil6a.tga
		blendfunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}

	{
		map textures/gothic_wall/window_evil6a_glow.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
	
}

textures/gothic_wall/window_evil7

{
	q3map_lightimage textures/gothic_wall/window_evil7_glow.tga
	q3map_surfacelight 300
	//light 1

	{
		map $lightmap
		rgbGen identity
	}
	
	{
		map textures/gothic_wall/window_evil7.tga
		blendfunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}

	{
		map textures/gothic_wall/window_evil7_glow.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
	
}


textures/gothic_wall/glass_stained1

{

	surfaceparm trans
	qer_editorimage	textures/gothic_wall/glass_stained1.tga

	
	

	{
		map textures/gothic_wall/glass_stained1.tga
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
	

}

textures/gothic_trim/gothgrate3
{
	surfaceparm	metalsteps		
	cull none

	// A GRATE OR GRILL THAT CAN BE SEEN FROM BOTH SIDES
	{
		map textures/gothic_trim/gothgrate3.tga
		blendFunc GL_ONE GL_ZERO
		rgbGen identity
		alphaFunc GT0
		depthWrite
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
		depthFunc equal
	}
}

textures/gothic_trim/gothgrate2
{
	surfaceparm	metalsteps		
	cull none

	// A GRATE OR GRILL THAT CAN BE SEEN FROM BOTH SIDES
	{
		map textures/gothic_trim/gothgrate2.tga
		blendFunc GL_ONE GL_ZERO
		rgbGen identity
		alphaFunc GT0
		depthWrite
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
		depthFunc equal
	}
}
textures/gothic_trim/tongue_bone
{
	{	
		map $lightmap
		rgbGen identity
	}
	{
		map textures/gothic_trim/tongue_bone.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/gothic_trim/tongue_bone.blend.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.5 0.5 0 0.2
//		rgbGen wave sin 0.5 0.5 0 0.2
	}
}

textures/gothic_trim/metalbase09d_iron
{
qer_editorimage	textures/gothic_trim/metalbase09_d.tga

	//	*************************************************
	//	*      	Tim's Border #1			

		{
			map textures/gothic_wall/iron01_e.tga
			// Name of background wall
			rgbGen identity
		}

		{	
			map textures/gothic_trim/metalbase09_d.tga
			// Name of border wall
			rgbGen identity
			blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		}

		{	
			map $lightmap
			rgbGen identity
			blendfunc GL_DST_COLOR GL_ZERO
		}				

	//	END
}

textures/gothic_trim/metalbase09e_iron
{
qer_editorimage	textures/gothic_trim/metalbase09_e.tga

	//	*************************************************
	//	*      	Tim's Borders			

		{
			map textures/gothic_wall/iron01_e.tga
			// Name of background wall
			rgbGen identity
		}

		{	
			map textures/gothic_trim/metalbase09_e.tga
			// Name of border wall
			rgbGen identity
			blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		}

		{	
			map $lightmap
			rgbGen identity
			blendfunc GL_DST_COLOR GL_ZERO
		}				

	//	END
}

textures/gothic_trim/metalbase02b_iron
{
qer_editorimage	textures/gothic_trim/metalbase02_b.tga

	//	*************************************************
	//	*      	Tim's Border #2			

		{
			map textures/gothic_wall/iron01_e.tga
			// Name of background wall
			rgbGen identity
		}

		{	
			map textures/gothic_trim/metalbase02_b.tga
			// Name of border wall
			blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
			rgbGen identity
		}

		{	
			map $lightmap
			blendfunc GL_DST_COLOR GL_ZERO
			rgbGen identity
		}				

	//	END
}

textures/gothic_trim/metalbase03c_iron
{
qer_editorimage	textures/gothic_trim/metalbase03_c.tga

	//	*************************************************
	//	*      	Tim's Border #3			

		{
			map textures/gothic_wall/iron01_e.tga
			rgbGen identity
			// Name of background wall
		}

		{	
			map textures/gothic_trim/metalbase03_c.tga
			// Name of border wall
			blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
			rgbGen identity
		}

		{	
			map $lightmap
			blendfunc GL_DST_COLOR GL_ZERO
			rgbGen identity
		}				

	//	END
}


textures/gothic_trim/metalbase02c_iron
{
qer_editorimage	textures/gothic_trim/metalbase02_c.tga

	//	*************************************************
	//	*      	Tim's Border #5 red			

		{
			map textures/gothic_wall/iron01_e.tga
			// Name of background wall
			rgbGen identity
		}

		{	
			map textures/gothic_trim/metalbase02_c.tga
			// Name of border wall
			blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
			rgbGen identity
		}

		{	
			map $lightmap
			blendfunc GL_DST_COLOR GL_ZERO
			rgbGen identity
		}				

	//	END
}

textures/gothic_trim/metalbase03b_iron
{
qer_editorimage	textures/gothic_trim/metalbase03_b.tga

	//	*************************************************
	//	*      	Tim's Border #6 red			

		{
			map textures/gothic_wall/iron01_e.tga
			// Name of background wall
			rgbGen identity
		}

		{	
			map textures/gothic_trim/metalbase03_b.tga
			// Name of border wall
			blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
			rgbGen identity
		}

		{	
			map $lightmap
			blendfunc GL_DST_COLOR GL_ZERO
			rgbGen identity
		}				

	//	END
}

textures/gothic_trim/metalbase09b_iron
{
qer_editorimage	textures/gothic_trim/metalbase09_b.tga

	//	*************************************************
	//	*      	Tim's Border #7 red			

		{
			map textures/gothic_wall/iron01_e.tga
			// Name of background wall
			rgbGen identity
		}

		{	
			map textures/gothic_trim/metalbase09_b.tga
			// Name of border wall
			blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
			rgbGen identity
		}

		{	
			map $lightmap
			blendfunc GL_DST_COLOR GL_ZERO
			rgbGen identity
		}				

	//	END
}

textures/gothic_trim/metalbase03a_brick
{
qer_editorimage	textures/gothic_trim/metalbase03_a.tga

	//	*************************************************
	//	*      	Tim's Border			

		{
			map textures/gothic_wall/streetbricks10.tga
			// Name of background wall
			tcMod scale 2 2
			rgbGen identity
		}

		{	
			map textures/gothic_trim/metalbase03_a.tga
			// Name of border wall
			blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
			rgbGen identity
		}

		{	
			map $lightmap
			blendfunc GL_DST_COLOR GL_ZERO
			rgbGen identity
		}				

	//	END
}

textures/gothic_trim/metalbase03d_iron
{
qer_editorimage	textures/gothic_trim/metalbase03_d.tga

	//	*************************************************
	//	*      	Tim's Border			

		{
			map textures/gothic_wall/iron01_e.tga
			// Name of background wall
			//tcMod scale 2 2 //this makes in normal size
			rgbGen identity
		}

		{	
			map textures/gothic_trim/metalbase03_d.tga
			// Name of border wall
			blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
			rgbGen identity
		}

		{	
			map $lightmap
			blendfunc GL_DST_COLOR GL_ZERO
			rgbGen identity
		}				

	//	END
}


