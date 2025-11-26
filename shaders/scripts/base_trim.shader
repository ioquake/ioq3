textures/base_trim/pj_launcher
{
        {
        	 map textures/base_trim/pj_launcher2.tga
        	 tcmod scroll 0 2
                 rgbGen identity
        }      
        {
        	 map textures/base_trim/pj_launcher.tga
        	 blendfunc blend
                 rgbGen identity
        }
        {
		map $lightmap
                blendFunc filter
		rgbGen identity
	}
}
textures/base_trim/pewter_nonsolid
{
	qer_editorimage textures/base_trim/dirty_pewter.tga
	surfaceparm nonsolid
	{
		map $lightmap
		rgbGen identity
	
	}
	{
		map textures/base_trim/dirty_pewter.tga
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO

	
	}
}

textures/base_trim/proto_comp
{
        {
        	 map textures/base_trim/proto_comp2.tga
        	 tcmod scroll 0 -.2
        	 rgbGen wave sin .5 .5 0 3
                 rgbGen identity
        }
        {
        	 clampmap textures/sfx/console03.tga
        	 tcmod rotate -33
                 tcMod stretch sin .8 0 0 0
                 blendfunc add
                 rgbGen wave sin .6 0 0 0
        }
        {
        	 clampmap textures/sfx/console02.tga
        	 tcmod rotate -333
                 tcMod stretch sin .8 0 0 0
                 blendfunc add
                 rgbGen wave sin .3 0 0 0
        }
        {
        	 map textures/effects/tinfx2c.tga
        	 tcGen environment
                 blendfunc add
                 rgbGen identity
        }
        {
        	 map textures/base_trim/proto_comp.tga
        	 blendfunc blend
        }
        {
		map $lightmap
                blendFunc filter
		rgbGen identity
	}
}

textures/base_trim/pewter_shiney
{   
 
        {
                map textures/effects/tinfx.tga       
                tcGen environment
                rgbGen identity
	}   
        {
		map textures/base_trim/pewter_shiney.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	} 
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
}

textures/base_trim/panel_glo
{   
    
        {
                map textures/sfx/firegorre2.tga       
                
                tcMod turb 0 .6 0 0.06
                tcmod scale .53 .61
                tcmod scroll 0 4
                blendFunc GL_ONE GL_ZERO
                rgbGen wave sin 1 1 0 .1
	}   
        {
		map textures/base_trim/panel_glo.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	} 
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
}
textures/base_trim/spidertrim4
{   
    
        {
                map textures/effects/tinfx.tga       
                tcGen environment
                rgbGen identity
	}   
        {
		map textures/base_trim/spidertrim4.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	} 
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
}
textures/base_trim/panel_shiny
{   
    
        {
                map textures/effects/tinfx.tga       
                tcGen environment
                rgbGen identity
	}   
        {
		map textures/base_trim/panel_shiny.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	} 
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
}
textures/base_trim/spidertrim3
{   
    
        {
                map textures/effects/tinfx.tga       
                tcGen environment
                rgbGen identity
	}   
        {
		map textures/base_trim/spidertrim3.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	} 
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
}
textures/base_trim/wires02
{
    cull disable
    surfaceparm alphashadow
    surfaceparm trans	
        {
                map textures/base_trim/wires02.tga
                alphaFunc GE128
		depthWrite
		rgbGen vertex
        }
        {
		map $lightmap
		rgbGen identity
		blendFunc filter
		depthFunc equal
	}


}
textures/base_trim/wires01
{
    cull disable
    surfaceparm alphashadow
    deformVertexes autoSprite2
    surfaceparm trans	
        {
                map textures/base_trim/wires01.tga
                alphaFunc GE128
		depthWrite
		rgbGen vertex
        }
        {
		map $lightmap
		rgbGen identity
		blendFunc filter
		depthFunc equal
	}


}
textures/base_trim/wires01_ass
{
    cull disable
    surfaceparm alphashadow
    deformVertexes autoSprite2
        {
                map textures/base_trim/wires01_ass.tga
                alphaFunc GE128
		depthWrite
		rgbGen vertex
        }
        {
		map $lightmap
		rgbGen identity
		blendFunc filter
		depthFunc equal
	}


}
textures/base_trim/spiderbite
{

        {
		map textures/sfx/proto_zzztblu2.tga
                tcMod turb 0 .3 0 .2
                tcmod scroll 6 .7
                tcmod scale .4 1
                blendFunc GL_ONE GL_ZERO
                rgbGen identity
	}
        {
	        map textures/base_trim/spiderbite.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
                tcMod stretch sin .8 0.4 0 .2
                tcmod scroll .3 0
	        rgbGen identity
	}
        {
	        map textures/base_trim/spiderbite.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
                tcMod stretch sin .8 0.2 0 .1
                tcmod scroll -.5 0
	        rgbGen identity
	}
	{
	        map textures/base_trim/spiderbite.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	        rgbGen identity
	}
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
}
textures/base_trim/dark_tin2
{   
   
        {
                map textures/effects/tinfx.tga       
                tcGen environment
                rgbGen vertex
	}  
        {
		map textures/base_trim/dark_tin2.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
         
}
textures/base_trim/dark_tin2_trans
{  
	qer_editorimage textures/base_trim/dark_tin2
	surfaceparm nonsolid
   
        {
                map textures/effects/tinfx.tga       
                tcGen environment
                rgbGen vertex
	}  
        {
		map textures/base_trim/dark_tin2.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
         
}

textures/base_trim/skull_monitor
{   
    
        {
                map textures/base_trim/skull_monitor.tga 
                tcMod turb 0 .05 0 .2
                rgbGen identity
	} 
        { 
                 map textures/sfx/text.tga
                 blendFunc GL_ONE GL_ONE
                 tcmod scroll 1 0
                 rgbGen identity
	}  
        { 
                 map textures/effects/tinfx2.tga
                 blendFunc GL_ONE GL_ONE
                 tcGen environment
                 rgbGen identity
	}  
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
         
}
textures/base_trim/deeprust_trans
{
	qer_editorimage textures/base_trim/deeprust.tga
	surfaceparm nonsolid
	{
		map $lightmap
		rgbGen identity
	
	}
	{
		map textures/base_trim/deeprust.tga
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO

	
	}
}

textures/base_trim/yellow_rustc
{
	
	{
		map textures/base_wall/chrome_env.tga
		tcgen environment
		tcmod scale .25 .25
		rgbGen identity
	}
	{
		map textures/base_trim/yellow_rustc.tga
		blendFunc gl_one_minus_src_alpha gl_src_alpha
	//	alphagen lightingspecular
		rgbGen identity
	}

	{
		map $lightmap
		blendfunc gl_dst_color gl_zero
		rgbGen identity
	}

	
       
}


textures/base_trim/panelbrn010w
{
	q3map_lightimage textures/base_trim/panelbrn010wfx.tga
	
        {
		map textures/base_trim/panelbrn010w.tga
        }
	
        {
		map textures/sfx/hologirl2.tga
                tcmod scale .4 .4
                tcmod scroll 6 2
                blendFunc GL_ONE GL_ONE
                rgbGen identity
	}
	{
		map textures/base_trim/panelbrn010w.tga
                blendfunc blend
		
	}
        {
		map $lightmap
		rgbGen identity
		blendFunc filter
	}
       
}
textures/base_trim/pewter_spec
{
	{
		rgbGen identity
		map $lightmap
	}
	{
		map textures/base_trim/pewter_spec.tga
		blendFunc GL_DST_COLOR GL_SRC_ALPHA
		rgbGen identity
		alphaGen lightingSpecular
	}
      
}
textures/base_trim/tin
{
	//deformVertexes wave 100 sin 3 2 .1 3
        {
		rgbGen identity
		map $lightmap
	}
	{
		map textures/base_trim/tin.tga
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
		map textures/base_trim/tin.tga
		//blendFunc GL_ONE GL_ONE
                blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}       
textures/base_trim/dark_tin
{
	//deformVertexes wave 100 sin 3 2 .1 3
        {
		rgbGen identity
		map $lightmap
	}
	{
		map textures/base_trim/dark_tin.tga
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
		map textures/base_trim/dark_tin.tga
		//blendFunc GL_ONE GL_ONE
                blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
       
              
}
textures/base_trim/techborder
{
	q3map_surfacelight 300	
        q3map_lightimage textures/base_trim/techborder_fx.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/base_trim/techborder.tga
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/base_trim/techborder_fx.tga
                //tcmod scale  .1 1
                tcmod scroll .1 0
                
		blendfunc GL_ONE GL_ONE
	}
        {
		map textures/base_trim/techborder_fx.tga
                tcmod scale  .1 1
                tcmod scroll .1 0
                
		blendfunc GL_ONE GL_ONE
	}
}
textures/base_trim/spiderbit
{
	q3map_lightimage textures/base_trim/spiderbit_fx.tga
	q3map_surfacelight 100
	
        {
		map $lightmap
		rgbGen identity
	}
        {
		map textures/base_trim/spiderbit.tga
		blendfunc gl_dst_color gl_zero
                rgbGen identity
	}	
        {
		map textures/base_trim/spiderbit_fx.tga
		blendfunc GL_ONE GL_ONE
                rgbgen wave triangle .1 .5 0 7
	}
    	
}
textures/base_trim/proto_fence
{
	surfaceparm trans		
	cull none
        nopicmip

	{
		map textures/base_trim/proto_fence.tga
		tcMod scale 3 3
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
textures/base_trim/border11light
{
	q3map_surfacelight 500
	Q3map_lightimage textures/base_trim/border11light.glow.tga	
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/base_trim/border11light.tga
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/base_trim/border11light.glow.tga
		blendfunc GL_ONE GL_ONE
		rgbgen wave sin .9 .1 0 10
	}
}

textures/base_trim/border11_250
{
	qer_editorimage textures/base_trim/border11light.tga
	q3map_surfacelight 250
	q3map_lightimage textures/base_trim/border11light.glow.tga	
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/base_trim/border11light.tga
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/base_trim/border11light.glow.tga
		blendfunc GL_ONE GL_ONE
		rgbgen wave sin .9 .1 0 10
	}
}

textures/base_trim/border12b
{
	q3map_surfacelight 1000	
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/base_trim/border12b.tga
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/base_trim/border12bfx.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/base_trim/border12b_pj
{
	qer_editorimage textures/base_trim/border12b.tga
	q3map_surfacelight 150	
	q3map_lightimage textures/base_trim/border12bfx.tga	
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/base_trim/border12b.tga
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/base_trim/border12bfx.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/base_trim/xborder12b
{
	qer_editorimage textures/base_trim/border12b.tga
	q3map_surfacelight 100	
	q3map_lightimage textures/base_trim/border12bfx.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/base_trim/border12b.tga
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/base_trim/border12bfx.tga
		blendfunc GL_ONE GL_ONE
	}
}


textures/base_trim/drag_glass1
{
// ************************************************************
// A reddish dragonish glass panel (temp)
// ************************************************************
	surfaceparm trans
	surfaceparm nolightmap
	{
		map textures/base_trim/drag_glass1.tga
		blendFunc GL_ONE_MINUS_DST_COLOR GL_ZERO
	}
	{
		map textures/base_trim/drag_glass1.tga
		alphaFunc LT128
		blendfunc GL_DST_COLOR GL_ONE
	}
}
textures/base_trim/electricwall	
{
	qer_editorimage textures/organics/wire02a_f.tga
	light 1	
q3map_surfacelight 400

	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/organics/wire02a_f.tga
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/organics/wire02a_f.tga
		blendFunc GL_ONE GL_ONE
	}
	{
		map textures/sfx/electric2.blend.tga
		rgbGen wave sin 0.6 1 0 2
	//	tcMod rotate 54
		tcMod scroll 0.53 0.43
		blendfunc GL_ONE GL_ONE
	}
}
//**********************************************************
//*    Electric wires with blue electical pattern dancing  *
//*    upon it giving off blue light			   *
//*    Created: 4/12/98 by Paul Jaquays                    *
//**********************************************************
textures/base_trim/electric_blue
{
	qer_editorimage textures/base_trim/electricwall_blue.tga
	q3map_lightimage textures/sfx/electric2.blend.tga
	q3map_globaltexture	
	q3map_surfacelight 200

	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/base_trim/electricwall_blue.tga
		tcMod scale 0.5 0.5
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/base_trim/electricwall_blue.tga
		blendFunc GL_ONE GL_ONE
	}
	{
		map textures/sfx/electric2.blend.tga
		rgbGen wave sin 0.6 1 0 2
		tcMod scale 0.5 0.5
		tcMod rotate 54
		tcMod scroll 0.53 0.43
		blendfunc GL_ONE GL_ONE
	}
}

