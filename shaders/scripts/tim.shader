//Bwahahahahhaha

textures/tim/red_flagbase
{      
      qer_editorimage textures/tim/flagbase.tga 
        
        {
	        map textures/tim/swirl_red.tga
                blendFunc GL_ONE GL_ZERO
                tcmod rotate 130
                tcMod stretch sin .7 0.5 0 .2
	        rgbGen identity
	}
        {
	        map textures/tim/swirl_red.tga
		blendFunc Add
                tcmod rotate 80
                tcMod stretch sin .7 0.5 0 .1
	        rgbGen identity
	}
        {
		map textures/tim/flagbase.tga
                blendFunc blend
		rgbGen identity
	}
       {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
}

textures/tim/blue_flagbase
{      
      qer_editorimage textures/tim/flagbase.tga 
        
        {
	        map textures/tim/swirl_blue.tga
                blendFunc GL_ONE GL_ZERO
                tcmod rotate 130
                tcMod stretch sin .7 0.5 0 .2
	        rgbGen identity
	}
        {
	        map textures/tim/swirl_blue.tga
		blendFunc Add
                tcmod rotate 80
                tcMod stretch sin .7 0.5 0 .1
	        rgbGen identity
	}
        {
		map textures/tim/flagbase.tga
                blendFunc blend
		rgbGen identity
	}
       {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
}
textures/tim/timborder11c
{
	q3map_surfacelight 300
	q3map_lightimage textures/tim/tim11c_pulse1.tga
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
		map textures/tim/tim11c_pulse1.tga
		blendfunc gl_one gl_one
		tcmod scale .035 1
		tcmod scroll -0.65 0


	}

	
}

textures/tim/rmetaltechgeomtrn
{


        {
		map textures/sfx/fire_ctfred.tga
                tcmod scroll 0 1
                tcMod turb 0 .25 0 1.6
                tcmod scale 2 2
                blendFunc GL_ONE GL_ZERO
                rgbGen identity
	}
        {
	        map textures/tim/rmetaltechgeomtrn.tga
                blendFunc blend
                tcmod rotate 30
                tcMod stretch sin .8 0.2 0 .2
	        rgbGen identity
	}
        {
	        map textures/base_wall/metaltechgeomtrn2.tga
		blendFunc blend
                tcmod rotate 20
                tcMod stretch sin .8 0.2 0 .1
	        rgbGen identity
	}
	{
	        map textures/tim/rmetaltechgeomtrn.tga
		blendFunc blend
	        rgbGen identity
	}
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
}

textures/tim/redmetalsupport2fline
{
	q3map_surfacelight 100
	q3map_lightimage textures/tim/redmetalsupport2fline_glow.tga

	{
		map textures/base_wall/chrome_env.tga
	        rgbGen identity
		tcGen environment
		tcmod scale .25 .25
	}
	
	{
		map textures/tim/redmetalsupport2fline.tga
		blendFunc blend	
		rgbGen identity
	}
	{
		map $lightmap
		blendfunc filter
		rgbGen identity
	}

	{
		map textures/tim/redmetalsupport2fline_glow.tga
		blendFunc add	
		rgbGen wave sin .5 .5 0 1	
	}


}
textures/tim/symbol_redlt1   
{
	q3map_lightimage textures/tim/symbol_redlt1_mask.tga
	q3map_surfacelight 100
	light 1

	{
		map $lightmap
		rgbGen identity
	}

	
	{
		map textures/tim/symbol_redlt1.tga
		blendfunc filter
		rgbgen identity
	}
	
	{
		map textures/tim/symbol_redlt1_mask.tga
		blendfunc add
		rgbgen wave sawtooth .2 .05 0 7
	}
}

textures/tim/rconcrete_ow
{       
        
        {
		map textures/tim/proto_zzztred.tga
                tcMod turb 0 .5 0 9.6
                tcmod scale 2 2
                tcmod scroll 9 5
                blendFunc GL_ONE GL_ZERO
                rgbGen identity
	}
        {
		map textures/tim/rconcrete_ow.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
        {
		map $lightmap
                blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
}