textures/common/nolightmap
{
	surfaceparm nolightmap
}

textures/common/nodrawnonsolid
{
	surfaceparm	nonsolid
	surfaceparm	nodraw
}

textures/common/invisible
{
	surfaceparm nolightmap			
        {
                map textures/common/invisible.tga
                alphaFunc GE128
		depthWrite
		rgbGen vertex
        }
}

textures/common/teleporter
{
	surfaceparm nolightmap
	surfaceparm noimpact
	q3map_lightimage textures/sfx/powerupshit.tga	
	q3map_surfacelight 800
	{
		map textures/sfx/powerupshit.tga
		tcGen environment
//		tcMod scale 5 5
		tcMod turb 0 0.015 0 0.3
	}
}

textures/common/nodrop
{
	qer_nocarve
	qer_trans	0.5
	surfaceparm 	trans
	surfaceparm	nonsolid
	surfaceparm	nomarks
	surfaceparm     nodrop
	surfaceparm 	nolightmap
	surfaceparm 	nodraw
	cull		disable
}

textures/common/weapclip
{
	qer_trans 0.40
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm nodraw
}

textures/common/clip
{
	qer_trans 0.40
	surfaceparm 	nolightmap
	surfaceparm nomarks
	surfaceparm nodraw
	surfaceparm nonsolid
        //surfaceparm 	nolightmap //proto_addition 11/08/99
	surfaceparm playerclip
	surfaceparm noimpact
}

textures/common/full_clip
{
	qer_trans 0.40
	surfaceparm nodraw
	surfaceparm playerclip
}


textures/common/missileclip
{
	qer_trans 0.40
	surfaceparm nodamage
	surfaceparm nomarks
	surfaceparm nodraw
	//surfaceparm nonsolid
	surfaceparm playerclip
}

//*****************************************
//**   I've reduced the transparency on most editor-only brushes here.
//**   If the transparent textures are still too pale for
//**   things like clip, hint, trigger, etc.,
//**   Please make an editorimage to suit your needs.
//******************************************

textures/common/cushion
{
	qer_nocarve
	qer_trans 0.50
	surfaceparm nodraw
	surfaceparm nomarks
	surfaceparm nodamage
	surfaceparm trans
}

textures/common/hint
{
qer_nocarve
qer_trans 0.30
surfaceparm nodraw
surfaceparm nonsolid
surfaceparm structural
surfaceparm trans
surfaceparm noimpact
}

textures/common/skip
{
qer_nocarve
qer_trans 0.40
surfaceparm nodraw
surfaceparm nonsolid
surfaceparm structural
surfaceparm trans
}

textures/common/slick
{
	qer_trans 0.50
	surfaceparm nodraw
	surfaceparm nomarks
	surfaceparm trans
	surfaceparm slick
}

textures/common/origin
{
	qer_nocarve
	surfaceparm nodraw
	surfaceparm nonsolid
	surfaceparm origin
}

textures/common/noimpact
{
	surfaceparm noimpact
}

textures/common/areaportal
{
	qer_trans 0.50
	surfaceparm nodraw
	surfaceparm nonsolid
	surfaceparm structural
	surfaceparm trans
	surfaceparm	nomarks
	surfaceparm areaportal
}

//bot specific cluster portal
textures/common/clusterportal
{
	qer_trans 0.50
	surfaceparm nodraw
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm	nomarks
	surfaceparm detail
	surfaceparm clusterportal
}

//bot specific "do not enter" brush
textures/common/donotenter
{
	qer_trans 0.50
	surfaceparm nodraw
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm	nomarks
	surfaceparm detail
	surfaceparm donotenter
}

textures/common/trigger
{
	qer_trans 0.50
	qer_nocarve
	surfaceparm nodraw
}

textures/common/caulk
{
	surfaceparm nodraw
	surfaceparm nomarks
        surfaceparm nolightmap
}

textures/common/nodraw
{
	surfaceparm nodraw
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nomarks
}

textures/common/mirror1
{
	qer_editorimage textures/common/qer_mirror.tga
	surfaceparm nolightmap
	portal
  
	{
		map textures/common/mirror1.tga
		blendfunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		depthWrite
	}
       
        

}

textures/common/mirror2
{
	qer_editorimage textures/common/qer_mirror.tga
	surfaceparm nolightmap
	portal
	{
		map textures/common/mirror1.tga
		blendfunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		depthWrite
	}
        {
               map textures/sfx/mirror.tga
	       blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        }

}
textures/common/energypad
{
	qer_editorimage textures/common/bluegoal.tga
	surfaceparm nolightmap
	cull twosided
	{
		map textures/common/bluegoal.tga
		blendFunc GL_ONE GL_SRC_ALPHA
		tcGen environment
		tcMod turb 0 0.25 0 0.05
	}
}

textures/common/portal
{
	qer_editorimage textures/common/qer_portal.tga
	surfaceparm nolightmap
	portal
	{
		map textures/common/mirror1.tga
//		map textures/common/portal.tga
		tcMod turb 0 0.25 0 0.05
//		blendFunc GL_ONE GL_SRC_ALPHA
		blendfunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		depthWrite

	}
}

textures/common/timportal
{
	qer_editorimage textures/common/qer_portal.tga
	portal
	surfaceparm nolightmap
	{
		map textures/common/portal.tga
		tcMod turb 0 0.25 0 0.05
		blendFunc GL_ONE GL_SRC_ALPHA
		depthWrite
	}
}

textures/sfx/portal_sfx
{

	//	*************************************************
	//	*      	Portal Inside Effect 			*
	//	*      	April 29				*	
	//	*	Please Comment Changes			*
	//	*************************************************

	portal
	surfaceparm nolightmap
	deformVertexes wave 100 sin 0 2 0 .5


	{
		map textures/sfx/portal_sfx3.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	//	This blend function is not working on RagePro drivers Mac only
	//	when it is used on top of portals
		depthWrite
	}


	{
		map textures/sfx/portal_sfx1.tga
		blendfunc gl_dst_color gl_zero
		tcMod rotate 360
	}

	{
		map textures/sfx/portal_sfx.tga
		blendfunc gl_one gl_one
		rgbgen wave inversesawtooth 0 .5 .2 .5
	}

	{
		map textures/sfx/portalfog.tga
		blendfunc gl_src_alpha gl_one_minus_src_alpha
		alphagen portal 256
		rgbGen identityLighting	
		tcmod turb sin 0 .5 0 1
		tcmod rotate .1 .1
		tcmod scroll .01 .03
	}
}
textures/sfx/portal2_sfx
{

	//	*************************************************
	//	*      	Portal Inside Effect 			*
	//	*      	April 29				*	
	//	*	Please Comment Changes			*
	//	*************************************************

	portal
	surfaceparm nolightmap
	deformVertexes wave 100 sin 0 2 0 .5


	{
		map textures/sfx/portal_sfx3.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	//	This blend function is not working on RagePro drivers Mac only
	//	when it is used on top of portals
		depthWrite
	}


	{
		map textures/sfx/portal_sfx1.tga
		blendfunc gl_dst_color gl_zero
		tcMod rotate 360
	}

	{
		map textures/sfx/portal_sfx.tga
		blendfunc gl_one gl_one
		rgbgen wave inversesawtooth 0 .5 .2 .5
	}

	{
		map textures/sfx/portalfog.tga
		blendfunc gl_src_alpha gl_one_minus_src_alpha
		//alphagen portal 128
		rgbGen identityLighting	
		tcmod turb sin 0 .5 0 1
		tcmod rotate .1 .1
		tcmod scroll .01 .03
	}
}