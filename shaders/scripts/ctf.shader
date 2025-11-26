textures/ctf/blue_telep
{ 
        cull disable	
        surfaceparm nomarks
        surfaceparm trans
        sort additive	

        {
	        
	        clampmap textures/ctf/blue_telep.tga
		blendFunc add
                //depthWrite
                //tcMod stretch sin .9 0.1 0 .5
                tcmod rotate 327
                rgbGen identity
	}
        {
	        clampmap textures/ctf/blue_telep2.tga
		blendFunc add
                //depthWrite
                //tcMod stretch sin .9 0.1 0 .1
                tcmod rotate -211
                rgbGen identity
        }
        {
	        clampmap textures/ctf/telep.tga
		alphaFunc GE128
                depthWrite
                tcmod rotate 20
	        rgbGen identity
	}
        {
	        clampmap textures/ctf/telep.tga
		alphaFunc GE128
                depthWrite
                tcMod stretch sin .7 0 0 0
                tcmod rotate -20
                rgbGen identity
	}
        {
		map $lightmap
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
		depthFunc equal
	}



}
textures/ctf/red_telep
{ 
        cull disable	
        surfaceparm nomarks
        surfaceparm trans

        {
	        
	        clampmap textures/ctf/red_telep.tga
		blendFunc add
                //tcMod stretch sin .9 0.1 0 .5
                tcmod rotate 327
                rgbGen identity
	}
        {
	        clampmap textures/ctf/red_telep2.tga
		blendFunc add
                //tcMod stretch sin .9 0.1 0 .1
                tcmod rotate -211
                rgbGen identity
        }
        {
	        clampmap textures/ctf/telep.tga
		alphaFunc GE128
                tcmod rotate 20
	        rgbGen identity
	}
        {
	        clampmap textures/ctf/telep.tga
		alphaFunc GE128
                tcMod stretch sin .7 0 0 0
                tcmod rotate -20
                rgbGen identity
	}
        {
		map $lightmap
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
		depthFunc equal
	}



}

textures/ctf/test2_trans
{
	qer_editorimage textures/ctf/test2.tga
	surfaceparm nonsolid
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/ctf/test2.tga
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
	}

}

textures/ctf/test2_r_trans
{
	qer_editorimage textures/ctf/test2_r.tga
	surfaceparm nonsolid
	{
		map $lightmap
		rgbGen identity
	
	}
	{
		map textures/ctf/test2_r.tga
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/ctf/killblockctf_r
{
   qer_editorimage   textures/gothic_block/blocks15cgeomtrn.tga

        {
		map textures/sfx/fire_ctfred.tga
                tcmod scroll 0 1
                tcMod turb 0 .25 0 1.6
                tcmod scale 2 2
                blendFunc GL_ONE GL_ZERO
                rgbGen identity
	}
        {
	        map textures/gothic_block/blocks18cgeomtrn2.tga
		//blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
                blendFunc blend
                tcmod rotate 30
                tcMod stretch sin .8 0.2 0 .2
	        rgbGen identity
	}
        {
	        map textures/gothic_block/blocks18cgeomtrn2.tga
		blendFunc blend
                tcmod rotate 20
                tcMod stretch sin .8 0.2 0 .1
	        rgbGen identity
	}
	{
	        map textures/gothic_block/blocks15cgeomtrn.tga
		blendFunc blend
	        rgbGen identity
	}
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
}
textures/ctf/killblockctf_b
{
   qer_editorimage   textures/gothic_block/killblockgeomtrn.tga

        {
		map textures/sfx/fire_ctfblue.tga
                tcmod scroll 0 1
                tcMod turb 0 .25 0 1.6
                tcmod scale 2 2
                blendFunc GL_ONE GL_ZERO
                rgbGen identity
	}
        {
	        map textures/gothic_block/blocks18cgeomtrn2.tga
		//blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
                blendFunc blend
                tcmod rotate 30
                tcMod stretch sin .8 0.2 0 .2
	        rgbGen identity
	}
        {
	        map textures/gothic_block/blocks18cgeomtrn2.tga
		blendFunc blend
                tcmod rotate 20
                tcMod stretch sin .8 0.2 0 .1
	        rgbGen identity
	}
	{
	        map textures/gothic_block/killblockgeomtrn.tga
		blendFunc blend
	        rgbGen identity
	}
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
}
textures/ctf/ctf_arch_b_shiny
{      
     qer_editorimage textures/ctf/ctf_arch_b.tga
	   {
		map textures/ctf/ctf_arch_b.tga
                blendFunc GL_ONE GL_ZERO
		rgbGen identity
	} 
        {
                map textures/sfx/specular.tga
                blendFunc GL_ONE GL_ONE
                tcGen environment
                tcmod scale 10 10
                rgbGen identity
	}  
        {
		map textures/ctf/ctf_arch_b.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
        {
		map $lightmap
		blendfunc gl_dst_color gl_zero
		rgbGen identity
	}
         
}

textures/ctf/ctf_arch_r_shiny
{
	qer_editorimage textures/ctf/ctf_arch_r.tga      
        {
		map textures/ctf/ctf_arch_r.tga
                blendFunc GL_ONE GL_ZERO
		rgbGen identity
	} 
        {
                map textures/sfx/specular.tga
                blendFunc GL_ONE GL_ONE
                tcGen environment
                tcmod scale 10 10
                rgbGen identity
	}  
        {
		map textures/ctf/ctf_arch_r.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
        {
		map $lightmap
		blendfunc gl_dst_color gl_zero
		rgbGen identity
	}
         
}

textures/gothic_door/door02_bred2_shiny
{              
qer_editorimage textures/gothic_door/door02_bred2.tga 
	{
		map $lightmap
		rgbGen identity
	}


        {
		map textures/gothic_door/door02_bred2.tga
                blendFunc GL_dst_color GL_SRC_ALPHA
		alphagen lightingspecular
		rgbGen identity
	}

}


textures/gothic_door/door02_eblue2_shiny
{              
qer_editorimage textures/gothic_door/door02_eblue2.tga

	{
		map $lightmap
		rgbGen identity
	}


        {
		map textures/gothic_door/door02_eblue2.tga
                blendFunc GL_dst_color GL_SRC_ALPHA
		alphagen lightingspecular
		rgbGen identity
	}

}



textures/ctf/red_scary4_d3fin
{              

	{
		map $lightmap
		rgbGen identity
	}


        {
		map textures/ctf/red_scary4_d3fin.tga
                blendFunc GL_dst_color GL_SRC_ALPHA
		alphagen lightingspecular
		rgbGen identity
	}

}

textures/ctf/ctf_tower_bluefin_shiny
{              
qer_editorimage textures/ctf/ctf_tower_bluefin.tga 
	{
		map $lightmap
		rgbGen identity
	}


        {
		map textures/ctf/ctf_tower_bluefin.tga
                blendFunc GL_dst_color GL_SRC_ALPHA
		alphagen lightingspecular
		rgbGen identity
	}

}


textures/ctf/ctf_tower_redfin_shiny
{              
qer_editorimage textures/ctf/ctf_tower_redfin.tga 
	{
		map $lightmap
		rgbGen identity
	}


        {
		map textures/ctf/ctf_tower_redfin.tga
                blendFunc GL_dst_color GL_SRC_ALPHA
		alphagen lightingspecular
		rgbGen identity
	}

}


textures/ctf/metalbase09_bluekillblock_shiny
{              
qer_editorimage textures/ctf/metalbase09_bluekillblock.tga 
	{
		map $lightmap
		rgbGen identity
	}


        {
		map textures/ctf/metalbase09_bluekillblock.tga
                blendFunc GL_dst_color GL_SRC_ALPHA
		alphagen lightingspecular
		rgbGen identity
	}

}

textures/ctf/red_scary4_cfin
{              

	{
		map $lightmap
		rgbGen identity
	}


        {
		map textures/ctf/red_scary4_cfin.tga
                blendFunc GL_dst_color GL_SRC_ALPHA
		alphagen lightingspecular
		rgbGen identity
	}

}


textures/ctf/red_scary4_csansfacefin
{              

	{
		map $lightmap
		rgbGen identity
	}


        {
		map textures/ctf/red_scary4_csansfacefin.tga
                blendFunc GL_dst_color GL_SRC_ALPHA
		alphagen lightingspecular
		rgbGen identity
	}

}



textures/ctf/tallblue_2_fin
{              

	{
		map $lightmap
		rgbGen identity
	}


        {
		map textures/ctf/tallblue_2_fin.tga
                blendFunc GL_dst_color GL_SRC_ALPHA
		alphagen lightingspecular
		rgbGen identity
	}

}

textures/ctf/tallblue_1L_fin_shiny
{              
qer_editorimage textures/ctf/tallblue_1L_fin.tga 
	{
		map $lightmap
		rgbGen identity
	}


        {
		map textures/ctf/tallblue_1R_fin.tga
                blendFunc GL_dst_color GL_SRC_ALPHA
		alphagen lightingspecular
		rgbGen identity
	}

}

textures/ctf/tallblue_1R_fin_shiny
{              
qer_editorimage textures/ctf/tallblue_1R_fin.tga 
	{
		map $lightmap
		rgbGen identity
	}


        {
		map textures/ctf/tallblue_1R_fin.tga
                blendFunc GL_dst_color GL_SRC_ALPHA
		alphagen lightingspecular
		rgbGen identity
	}

}



textures/ctf/tallblue_2b_fin
{              

	{
		map $lightmap
		rgbGen identity
	}


        {
		map textures/ctf/tallblue_2b_fin.tga
                blendFunc GL_dst_color GL_SRC_ALPHA
		alphagen lightingspecular
		rgbGen identity
	}

}

textures/ctf/tallblue_4c_fin_shiny
{              
qer_editorimage textures/ctf/tallblue_4c_fin.tga 

	{
		map $lightmap
		rgbGen identity
	}


        {
		map textures/ctf/tallblue_4c_fin.tga
                blendFunc GL_dst_color GL_SRC_ALPHA
		alphagen lightingspecular
		rgbGen identity
	}

}




textures/ctf/tallblue_4bsmall_fin
{              

	{
		map $lightmap
		rgbGen identity
	}


        {
		map textures/ctf/tallblue_4bsmall_fin.tga
                blendFunc GL_dst_color GL_SRC_ALPHA
		alphagen lightingspecular
		rgbGen identity
	}

}


textures/ctf/tallblue_5esmallb_fin
{              

	{
		map $lightmap
		rgbGen identity
	}


        {
		map textures/ctf/tallblue_5esmallb_fin.tga
                blendFunc GL_dst_color GL_SRC_ALPHA
		alphagen lightingspecular
		rgbGen identity
	}

}

textures/ctf/ctf_blueflag
{
        tessSize 64
        deformVertexes wave 194 sin 0 3 0 .4
        deformVertexes normal .5 .1
        surfaceparm nomarks
        cull none
        
        {
		map textures/ctf/ctf_blueflag.tga
		rgbGen identity
	}
        {
		map textures/effects/redflagmap.tga
                tcGen environment
                tcmod scale 9 3
                tcmod scroll .1 .7
                
                blendFunc GL_ONE GL_ONE
                rgbGen identity
	}
        {
		map textures/ctf/ctf_blueflag.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
        {
        	map textures/sfx/shadow.tga
                tcGen environment 
                //blendFunc GL_ONE GL_ONE            
                blendFunc GL_DST_COLOR GL_ZERO
               rgbGen identity
	}
}
textures/ctf/ctf_redflag
{
        tessSize 64
        deformVertexes wave 194 sin 0 3 0 .4
        deformVertexes normal .3 .2
        surfaceparm nomarks
        cull none

        {
		map textures/ctf/ctf_redflag.tga
		rgbGen identity
	}
        {
		map textures/effects/blueflagmap.tga
                 tcGen environment
                tcmod scale 9 3
                tcmod scroll .1 .7
                
                blendFunc GL_ONE GL_ONE
                rgbGen identity
	}
        {
		map textures/ctf/ctf_redflag.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
        {

        	map textures/sfx/shadow.tga
                tcGen environment 
                //blendFunc GL_ONE GL_ONE            
                blendFunc GL_DST_COLOR GL_ZERO
               rgbGen identity
	}
}
powerups/redflag
{
	deformVertexes wave 100 sin 3 0 0 0
	{
		map textures/effects/redflagmap.tga
		blendfunc GL_ONE GL_ONE
		tcGen environment
		tcMod turb 0 0.2 0 1
	}
}

powerups/blueflag
{
	deformVertexes wave 100 sin 3 0 0 0
	{
		map textures/effects/blueflagmap.tga
		blendfunc GL_ONE GL_ONE
		tcGen environment
		tcMod turb 0 0.2 0 1
	}
}

sprites/friend
{
	nomipmaps
	nopicmip
	{
		map sprites/friend1.tga
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

sprites/foe
{
	nomipmaps
	nopicmip
	{
		map sprites/foe2.tga
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

