
textures/gothic_floor/q1metal7_99spot
{

        {
		map textures/liquids/proto_grueldark2.tga
                blendFunc GL_ONE GL_ZERO
                tcmod scale 2 2
                tcMod scroll .01 .03
                tcMod turb 0 0.05 0 .05
        }
        {
		map textures/effects/tinfx3.tga
                blendFunc GL_ONE GL_ONE
                tcGen environment
        }
       
	{
	        map textures/gothic_floor/q1metal7_99spot.tga
		blendFunc blend
	        rgbGen identity
	}
        {
		map $lightmap
                blendFunc filter
		rgbGen identity
	}
}

textures/gothic_floor/xq1metalbig_ow
{
        {
	        map textures/gothic_floor/xq1metalbig_ow.tga
	        rgbGen identity
	}
        {
		map textures/sfx/specular.tga
                //map textures/liquids/proto_gruel3.tga
                tcGen environment
                tcmod scale 3 3
                //tcmod rotate 30
                tcMod turb 0 .25 0 .05
                //tcmod scroll .1 .2
                blendFunc GL_ONE GL_ONE
                rgbGen identity
	}
	{
	        map textures/gothic_floor/xq1metalbig_ow.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	        rgbGen identity
	}
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
}
textures/gothic_floor/q1metal7_98blueow
{
        {
	        map textures/gothic_floor/q1metal7_98blueow.tga
	        rgbGen identity
	}
        {
		map textures/sfx/fire_ctfred.tga
                tcmod scroll 0 1
                tcMod turb 0 .25 0 1.6
                tcmod scale 4 4
                blendFunc GL_ONE GL_ONE
                rgbGen identity
	}
	{
	        map textures/gothic_floor/q1metal7_98blueow.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	        rgbGen identity
	}
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
}
textures/gothic_floor/q1metal7_97redow
{
        {
	        map textures/gothic_floor/q1metal7_97redow.tga
	        rgbGen identity
	}
        {
		map textures/sfx/fire_ctfblue.tga
                tcmod scroll 0 1
                tcMod turb 0 .25 0 1.6
                tcmod scale 4 4
                blendFunc GL_ONE GL_ONE
                rgbGen identity
	}
	{
	        map textures/gothic_floor/q1metal7_97redow.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	        rgbGen identity
	}
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
}
textures/gothic_floor/fireblocks17floor3
{
//	surfaceparm nolightmap
        {
		map textures/liquids/protolava2.tga
                tcmod scale .2 .2
                tcmod scroll .04 .03
                tcMod turb 0 .1 0 .01
                blendFunc GL_ONE GL_ZERO
                rgbGen identity
	}
	{
		map textures/gothic_floor/fireblocks17floor3.tga
                blendfunc blend
	}
	{
		map $lightmap
		blendfunc filter
	}
}

textures/gothic_floor/center2trn
{

        {
		map textures/sfx/fireswirl2.tga
                tcmod rotate 333
                tcMod stretch sin .8 0.2 0 9.7
	        rgbGen identity
	}
        {
	        map textures/gothic_floor/center2trn.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
                tcmod rotate 30
                tcMod stretch sin .8 0.2 0 .2
	        rgbGen identity
	}
        {
	        map textures/gothic_floor/center2trn.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	        rgbGen identity
	}
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
}
textures/gothic_floor/largerblock3b_ow
{

        {
		map textures/sfx/firegorre.tga
                tcmod scroll 0 1
                tcMod turb 0 .25 0 1.6
                tcmod scale 4 4
                blendFunc GL_ONE GL_ZERO
                rgbGen identity
	}
	{
	        map textures/gothic_floor/largerblock3b_ow.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	        rgbGen identity
	}
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
}
textures/gothic_floor/metalbridge06brokeb
{
        {
                map textures/sfx/kenelectric.tga  
                tcmod scale 2 2
                tcMod turb 0 .2 0 .3
                tcmod scroll 3 5
                rgbGen identity
	}  
        {
		map textures/gothic_floor/metalbridge06brokeb.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	} 
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
   
         
}

textures/gothic_floor/xstepborder3_shiney
{
	qer_editorimage textures/gothic_floor/xstepborder3_alpha.tga
	{
		map $lightmap
		rgbgen identity      
	}
	
	{
		map textures/gothic_floor/xstepborder3_alpha.tga
		blendFunc GL_DST_COLOR GL_SRC_ALPHA
		rgbGen identity
		alphaGen lightingSpecular
	}

			
}


textures/gothic_floor/xlargeblockfloor5_pent
{
	{
		map textures/gothic_floor/xlargeblockfloor5_pent.tga
		rgbGen identity
	}
	{
		map $lightmap
		rgbGen identity
		blendfunc gl_dst_color gl_zero
	}
	{
		map textures/sfx/pentfloor_256_glow.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .5 .5 0 .5	
		depthfunc equal
	}
}

textures/gothic_floor/xlargeblockfloor5_pent_offset
{
	{
		map textures/gothic_floor/xlargeblockfloor5_pent_offset.tga
		rgbGen identity
	}
	{
		map $lightmap
		rgbGen identity
		blendfunc gl_dst_color gl_zero
	}
	{
		map textures/sfx/pentfloor_256_glow.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .5 .5 0 .5	
		depthfunc equal
	}
}

textures/gothic_floor/glowfloor1
{
	q3map_lightimage textures/gothicfloor/glowfloor1.tga
	q3map_surfacelight 600

	// GLOWING RED Floor Color on blocks17g floor
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/gothic_block/blocks17g.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/gothic_floor/glowred2.blend.tga
		// rgbGen wave sin 0.5 0.5 0 0.5
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.5 0.1 1 0.5
	}
}

textures/gothic_floor/pent_metalbridge02
{
	q3map_lightimage textures/gothic_floor/pent_glow.tga
	q3map_surfacelight 500

	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/gothic_floor/pent_metalbridge02.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/gothic_floor/pent_glow.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .5 .5 0 0.5
	}
}
textures/gothic_floor/pent_metalbridge06
{
	q3map_lightimage textures/gothic_floor/pent_glow.tga
	q3map_surfacelight 500

	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/gothic_floor/pent_metalbridge06.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/gothic_floor/pent_glow.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .5 .5 0 0.5
	}
}

textures/gothic_floor/pent_metalbridge06b
{
	q3map_lightimage textures/gothic_floor/pent_glow.tga
	q3map_surfacelight 500

	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/gothic_floor/pent_metalbridge06b.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/gothic_floor/pent_glow.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .5 .5 0 0.5
	}
}

