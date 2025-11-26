textures/eerie/floor3_3dark
{
	surfaceparm	metalsteps
}


textures/eerie/xblocks11be
{
	qer_editorimage textures/eerie/blocks11b.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/blocks11b.tga
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/eerie/electricgrade3.tga
		rgbGen wave sin .25 .25 0.25 .25
		tcMod turb .2 .2 .1 1
		tcMod scale .25 .25
		tcMod scroll -.5 -1.2
		blendfunc GL_DST_COLOR GL_ONE
	}
}

textures/eerie/blackness
{
	surfaceparm nolightmap
	surfaceparm noimpact
	{
		map gfx/colors/black.tga
	}
}
textures/eerie/surface6jumppad
{
	{
		rgbGen identity
		map $lightmap
	}
	{
		rgbGen identity
		map textures/eerie/surface6.tga
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/eerie/surface6jumppad.blend.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.5 0.5 0 .2
//		blendFunc GL_ONE GL_ONE
//		rgbGen wave sin 0.75 0.25 0 .2
//		tcMod stretch sin 0.80 0.20 0 1
	}
}
textures/eerie/xgoopjumpypad
{
	q3map_surfacelight 100
	q3map_lightimage textures/eerie/surface6jumppad.blend.tga
	{
		rgbGen identity
		map $lightmap
	}
	{
		rgbGen identity
		map textures/eerie/xgoopq1metal.tga
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		clampmap textures/eerie/surface6jumppad.blend.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.75 0.25 0 .2
//		tcMod stretch sin 0.80 0.20 0 1
	}
}
textures/eerie/xgoopy_pentagram
{
//	qer_editorimage textures/eerie/xgoopy_pentagram.tga
	q3map_surfacelight 250
//	q3map_lightimage textures/eerie/pentagramfloor.red.blend.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/xgoopq1metal.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		clampmap textures/eerie/pentagramfloor.red.blend.tga
		blendFunc GL_ONE GL_ONE
//		rgbGen wave sin 0.5 0.5 0.3 .5
		tcMod rotate -60
//		blendFunc GL_ONE GL_ONE
//		rgbGen wave sin 0.5 0.5 0 .5
//		rgbGen wave sin 0.75 0.25 0 .2
//		tcMod stretch sin 0.80 0.20 0 1

	}
}

textures/eerie/gkcspine
{
	deformVertexes wave 100 sin 2 3 0 0.4
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/gkcspine.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/eerie/lavafogshader
{
	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm	nolightmap
	fogonly
}

textures/eerie/lavahell
{
	qer_editorimage textures/eerie/lavahell.tga
	qer_nocarve
	surfaceparm noimpact
	surfaceparm lava
	surfaceparm nonsolid
	surfaceparm nolightmap
	q3map_surfacelight 500
	tesssize 256
	cull disable
	deformVertexes wave 100 sin 5 5 .5 0.02
	fogparms ( 0.8519142 0.309723 0.0 ) 128 128

	{
		map textures/eerie/lavahell.tga
		tcMod turb .25 0.4 0 0.09
	}
}
textures/eerie/lavahell2
{
	qer_editorimage textures/eerie/lavahell.tga
	qer_nocarve
	surfaceparm noimpact
	surfaceparm lava
	surfaceparm nolightmap
	q3map_surfacelight 3000
	tesssize 256
	cull disable
	deformVertexes wave 100 sin 5 5 .5 0.02
	fogparms ( 0.8519142 0.309723 0.0 ) 128 128

	{
		map textures/eerie/lavahell.tga
		tcMod turb .25 0.2 1 0.02
		tcMod scroll 0.1 0.1
	}
}
textures/eerie/lavahell_goddamnbright
{
	qer_editorimage textures/eerie/lavahell.tga
	qer_nocarve
	surfaceparm noimpact
	surfaceparm lava
	surfaceparm nolightmap
	q3map_surfacelight 7000
	tesssize 256
	cull disable
	deformVertexes wave 100 sin 5 5 .5 0.02
	fogparms ( 0.8519142 0.309723 0.0 ) 128 128

	{
		map textures/eerie/lavahell.tga
		tcMod turb .25 0.2 1 0.02
		tcMod scroll 0.1 0.1
	}
}


textures/eerie/uberiron
{
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/uberiron.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/eerie/firewall_light.tga
		blendfunc gl_dst_color gl_zero
		tcMod scroll 5 15
		tcMod scale 0.25 0.25
		tcMod turb .2 .2 .1 1
	}
}

textures/eerie/iron01_e_flicker
{
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/iron01_e_flicker.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/eerie/firewalla.tga
		blendfunc gl_dst_color gl_zero
		tcMod scale 0.25 0.25
		tcMod turb .2 .2 .1 1
		tcMod scroll -15 -5
	}
	{
		map textures/eerie/firewallb.tga
		blendFunc GL_dst_color gl_src_alpha
		tcMod scale 0.1 0.1
		tcMod turb .2 .1 .1 1
		tcMod scroll -10 0
	}
}
textures/eerie/q1metal7_98d_flicker
{
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/q1metal7_98d.tga
		rgbGen identity
		blendfunc gl_dst_color gl_zero
	}
	{
		map textures/eerie/firewalla.tga
		blendfunc gl_dst_color gl_zero
		tcMod scroll -15 -5
		tcMod scale 0.25 0.25
		tcMod turb .2 .2 .1 1
	}
}

textures/eerie/iron01_d_flicker
{
	tesssize 32
	surfaceparm nolightmap
	{
		map textures/eerie/iron01_d_flicker.tga
	}
	{
		map textures/eerie/firewalla.tga
		blendfunc gl_dst_color gl_zero
		tcMod scroll 5 15
		tcMod scale 0.25 0.25
		tcMod turb .2 .2 .1 1
	}
	{
		map textures/eerie/firewallb.tga
		blendFunc GL_dst_color gl_src_alpha
		tcMod scale .1 .1 0 10
		tcMod turb .2 .1 .1 1
		tcMod scroll 0 10
	}
}
textures/eerie/iron01_g_flicker
{
	tesssize 32
	surfaceparm nolightmap
	{
		map textures/eerie/iron01_g_flicker.tga
	}
	{
		map textures/eerie/firewalla.tga
		blendfunc gl_dst_color gl_zero
		tcMod scale 0.25 0.25
		tcMod turb .2 .2 .1 1
		tcMod scroll 5 15
	}
	{
		map textures/eerie/firewallb.tga
		blendFunc GL_dst_color gl_src_alpha
		tcMod scale 0.1 0.1
		tcMod turb .2 .1 .1 1
		tcMod scroll 0 10
	}
}


textures/eerie/firewall
{
	surfaceparm nonsolid
	surfaceparm nolightmap
	q3map_surfacelight 6000
	tesssize 64
	{
		map textures/eerie/firewalla.tga
		blendfunc gl_one gl_zero
		tcMod scale 0.25 0.25
		tcMod turb .2 .2 .1 1
		tcMod scroll 5 15
	}
	{
		map textures/eerie/firewallb.tga
		blendFunc GL_dst_color gl_src_alpha
		tcMod scale .1 .1
		tcMod turb .2 .1 .1 1
		tcMod scroll 0 10
	}
	{
		map textures/eerie/firewall.tga
		blendFunc GL_dst_color gl_one
		tcMod turb .1 .1 .5 .5
		tcMod scroll 10 40
	}
}

textures/eerie/xq1metalbig_nojumppad
{
	surfaceparm nodamage
	{
		map $lightmap
		rgbGen identity
	}
	{
		rgbGen identity
		map textures/eerie/xq1metalbig_nojumppad.tga
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/eerie/xq1metalbig_nojumppad.blend.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.5 0.5 0 .2
	}
}

textures/eerie/metal3_3
{
	surfaceparm metalsteps
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/metal3_3.tga
		rgbGen identity
		blendfunc GL_DST_COLOR GL_ZERO
	}

}

textures/eerie/teslacoil
{
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/cabletest2.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/eerie/electricgrade3.tga

		rgbGen wave sin .25 .25 0.25 .25

		tcMod turb .2 .2 .1 1
		tcMod scale .25 .25
		tcMod scroll -.5 -1.2

		blendfunc GL_ONE GL_ONE
	}
	{
		map textures/eerie/electricslime.tga
		tcMod turb .2 .2 .1 1
		tcMod scale .25 .25
		tcMod scroll .5 1.5
		blendfunc GL_DST_COLOR GL_ONE

	}
}
textures/eerie/tesla_cable
{
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/tesla_cable.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/eerie/electricslime.tga
		tcMod turb .2 .2 1 .1
		tcMod scroll 0 15
		tcMod scale .1 .1
		blendfunc GL_DST_COLOR GL_ONE
	
//		map textures/eerie/electricslime.tga
//		tcMod turb 1 1 1 .01
//		tcMod scroll 1 5
//		tcMod scale .5 .5
//		blendfunc GL_ONE GL_ONE
	}
}
textures/eerie/jesus
{
	{
		rgbGen identity
		map $lightmap
	}
	{
		map textures/eerie/jesus.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/eerie/jesus_mask3.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}
textures/eerie/xq1metalbig_jumppad
{
	surfaceparm nodamage
	q3map_surfacelight 250
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/q1metal7_98d_256x256.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		clampmap textures/eerie/surface6jumppad.blend.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.75 0.25 0 .2
//		tcMod stretch sin 0.80 0.20 0 1
//		map textures/eerie/jumppad.tga
//		blendFunc GL_ONE GL_ONE
//		rgbGen wave sin 0.5 0.5 0 .2
	}
}
textures/eerie/blocks15b_jumppad
{
	q3map_surfacelight 250
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/blocks15b_jumppad.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/eerie/jumppad.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.5 0.5 0 .2
	}
}
textures/eerie/blocks11b_jumppad
{
	surfaceparm nodamage
	q3map_surfacelight 250
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/castle/blocks11b.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		clampmap textures/eerie/jumppad.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.75 0.25 0 .2
//		tcMod stretch sin 0.80 0.20 0 1
	}
}
textures/eerie/blocks11b_jumppad_smaller
{
	surfaceparm nodamage
	q3map_surfacelight 250
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/blocks11b_jumppad_smaller.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/eerie/jumppad_smaller.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.5 0.5 0 .2
	}
}


textures/eerie/pentagramfloor
{
//	q3map_surfacelight 250
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/pentagramfloor.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/eerie/pentagramfloor.red.blend.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.5 0.5 0 .5
	}
}
textures/eerie/pentagramfloor_rotating
{
	qer_editorimage textures/eerie/pentagramfloor.tga
//	q3map_surfacelight 250
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/xhugefloor2.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		clampmap textures/eerie/pentagramfloor.red.blend.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.75 0.25 0 0.5
		tcMod rotate -60
	}
}


textures/eerie/q1metal7_98d_pent_256
{
	q3map_surfacelight 250
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/q1metal7_98d_pent_256.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/eerie/pentagramfloor_256x256.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.5 0.5 0 .5
	}
}


textures/eerie/border11light
{
	q3map_surfacelight 1000	
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/border11light.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/eerie/border11light.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}
textures/eerie/borderlight_500
{
	q3map_surfacelight 500
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/border11light.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/eerie/border11light.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/eerie/darkness
{
	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm	nolightmap
	surfaceparm 	nodrop
	cull back
	fogonly
	fogparms ( 0 0 0 ) 128 128
}
textures/eerie/redgoal
{
	q3map_surfacelight 200

	{
		map textures/eerie/redgoal.tga
		tcGen environment
		blendfunc GL_ONE GL_ONE
		tcMod turb 0 0.25 0 0.05
	}
}
textures/eerie/bluegoal
{
	q3map_surfacelight 200
	{
		map textures/eerie/bluegoal.tga
		tcGen environment
		blendfunc GL_ONE GL_ONE
		tcMod turb 0 0.25 0 0.05
	}
}
textures/eerie/powerupshit
{
	surfaceparm nolightmap
	surfaceparm nonsolid
	cull twosided
	{
		map textures/eerie/powerupshit.tga
		tcGen environment
		blendfunc GL_ONE GL_ONE
		tcMod turb 0 0.25 0 0.5
	}
}
textures/eerie/xian_rune11
{
	q3map_surfacelight 1000
	light 1
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/xian_rune11.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/eerie/xian_rune11.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/eerie/xian_rune21
{
	q3map_surfacelight 1000
	light 1
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/xian_rune21.tga
		//map textures/eerie/xian_rune21.specular.tga
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/eerie/xian_rune21.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}
textures/eerie/lightcone
{
	{
		map textures/eerie/lightcone.tga
		tcGen environment
		blendfunc GL_ONE GL_ONE
	}
}

textures/eerie/q1metal7_97b_mirror
{
	surfaceparm nolightmap
	portal
	{
		map textures/eerie/q1metal7_97b_mirror.tga
		blendfunc GL_ONE GL_SRC_ALPHA
		depthWrite
	}
}
textures/eerie/cross_1000
{
	q3map_surfacelight 1000
	light 1
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/cross_1000.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/eerie/cross_1000.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}
textures/eerie/cross_2500
{
	q3map_surfacelight 2500
	light 1
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/cross_2500.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/eerie/cross_2500.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/eerie/light1
{
	q3map_surfacelight 2000
	q3map_lightimage textures/eerie/light1.blend.tga
	light 1
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/light1.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{	
		map textures/eerie/light1.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/eerie/light1_5000
{
	qer_editorimage textures/eerie/light1.tga
	q3map_lightimage textures/eerie/light1.blend.tga
	q3map_surfacelight 5000
	light 1
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/light1.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{	
		map textures/eerie/light1.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/eerie/light2
{
	q3map_surfacelight 4000
	light 1
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/light2.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/eerie/light2.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/eerie/light3_8
{
	q3map_surfacelight 4000
	light 1
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/light3_8.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/eerie/light3_8.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}
textures/eerie/xian_light3_7
{
	q3map_surfacelight 5000
	light 1
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/eerie/xian_light3_7.tga
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/eerie/xian_light3_7.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}
