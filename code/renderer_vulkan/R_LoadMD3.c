
#include "tr_local.h"
#include "tr_model.h"
#include "ref_import.h"
#include "tr_shader.h"
#define	LL(x) x=LittleLong(x)


static qboolean R_LoadMD3 (model_t *mod, int lod, void *buffer, const char *mod_name )
{
	int					i, j;
	md3Header_t			*pinmodel;
    md3Frame_t			*frame;
	md3Surface_t		*surf;
	md3Shader_t			*shader;
	md3Triangle_t		*tri;
	md3St_t				*st;
	md3XyzNormal_t		*xyz;
	md3Tag_t			*tag;
	int					version;
	int					size;

	pinmodel = (md3Header_t *)buffer;

	version = LittleLong (pinmodel->version);
	if (version != MD3_VERSION) {
		ri.Printf( PRINT_WARNING, "R_LoadMD3: %s has wrong version (%i should be %i)\n",
				 mod_name, version, MD3_VERSION);
		return qfalse;
	}

	mod->type = MOD_MESH;
	size = LittleLong(pinmodel->ofsEnd);
	mod->dataSize += size;
	mod->md3[lod] = ri.Hunk_Alloc( size, h_low );

	memcpy (mod->md3[lod], buffer, LittleLong(pinmodel->ofsEnd) );

    LL(mod->md3[lod]->ident);
    LL(mod->md3[lod]->version);
    LL(mod->md3[lod]->numFrames);
    LL(mod->md3[lod]->numTags);
    LL(mod->md3[lod]->numSurfaces);
    LL(mod->md3[lod]->ofsFrames);
    LL(mod->md3[lod]->ofsTags);
    LL(mod->md3[lod]->ofsSurfaces);
    LL(mod->md3[lod]->ofsEnd);

	if ( mod->md3[lod]->numFrames < 1 ) {
		ri.Printf( PRINT_WARNING, "R_LoadMD3: %s has no frames\n", mod_name );
		return qfalse;
	}
    
	// swap all the frames
    frame = (md3Frame_t *) ( (byte *)mod->md3[lod] + mod->md3[lod]->ofsFrames );
    for ( i = 0 ; i < mod->md3[lod]->numFrames ; i++, frame++) {
    	frame->radius = LittleFloat( frame->radius );
        for ( j = 0 ; j < 3 ; j++ ) {
            frame->bounds[0][j] = LittleFloat( frame->bounds[0][j] );
            frame->bounds[1][j] = LittleFloat( frame->bounds[1][j] );
	    	frame->localOrigin[j] = LittleFloat( frame->localOrigin[j] );
        }
	}

	// swap all the tags
    tag = (md3Tag_t *) ( (byte *)mod->md3[lod] + mod->md3[lod]->ofsTags );
    for ( i = 0 ; i < mod->md3[lod]->numTags * mod->md3[lod]->numFrames ; i++, tag++) {
        for ( j = 0 ; j < 3 ; j++ ) {
			tag->origin[j] = LittleFloat( tag->origin[j] );
			tag->axis[0][j] = LittleFloat( tag->axis[0][j] );
			tag->axis[1][j] = LittleFloat( tag->axis[1][j] );
			tag->axis[2][j] = LittleFloat( tag->axis[2][j] );
        }
	}

	// swap all the surfaces
	surf = (md3Surface_t *) ( (byte *)mod->md3[lod] + mod->md3[lod]->ofsSurfaces );
	for ( i = 0 ; i < mod->md3[lod]->numSurfaces ; i++) {

        LL(surf->ident);
        LL(surf->flags);
        LL(surf->numFrames);
        LL(surf->numShaders);
        LL(surf->numTriangles);
        LL(surf->ofsTriangles);
        LL(surf->numVerts);
        LL(surf->ofsShaders);
        LL(surf->ofsSt);
        LL(surf->ofsXyzNormals);
        LL(surf->ofsEnd);
		
		if ( surf->numVerts >= SHADER_MAX_VERTEXES ) {
			ri.Printf(PRINT_WARNING, "R_LoadMD3: %s has more than %i verts on %s (%i).\n",
				mod_name, SHADER_MAX_VERTEXES - 1, surf->name[0] ? surf->name : "a surface",
				surf->numVerts );
			return qfalse;
		}
		if ( surf->numTriangles*3 >= SHADER_MAX_INDEXES ) {
			ri.Printf(PRINT_WARNING, "R_LoadMD3: %s has more than %i triangles on %s (%i).\n",
				mod_name, ( SHADER_MAX_INDEXES / 3 ) - 1, surf->name[0] ? surf->name : "a surface",
				surf->numTriangles );
			return qfalse;
		}
	
		// change to surface identifier
		surf->ident = SF_MD3;

		// lowercase the surface name so skin compares are faster
		Q_strlwr( surf->name );

		// strip off a trailing _1 or _2
		// this is a crutch for q3data being a mess
		j = strlen( surf->name );
		if ( j > 2 && surf->name[j-2] == '_' ) {
			surf->name[j-2] = 0;
		}

        // register the shaders
        shader = (md3Shader_t *) ( (byte *)surf + surf->ofsShaders );
        for ( j = 0 ; j < surf->numShaders ; j++, shader++ ) {
            shader_t* sh = R_FindShader( shader->name, LIGHTMAP_NONE, qtrue );
			
            if ( sh->defaultShader ) {
				shader->shaderIndex = 0;
			} else {
				shader->shaderIndex = sh->index;
			}
        }

		// swap all the triangles
		tri = (md3Triangle_t *) ( (byte *)surf + surf->ofsTriangles );
		for ( j = 0 ; j < surf->numTriangles ; j++, tri++ ) {
			LL(tri->indexes[0]);
			LL(tri->indexes[1]);
			LL(tri->indexes[2]);
		}

		// swap all the ST
        st = (md3St_t *) ( (byte *)surf + surf->ofsSt );
        for ( j = 0 ; j < surf->numVerts ; j++, st++ ) {
            st->st[0] = LittleFloat( st->st[0] );
            st->st[1] = LittleFloat( st->st[1] );
        }

		// swap all the XyzNormals
        xyz = (md3XyzNormal_t *) ( (byte *)surf + surf->ofsXyzNormals );
        for ( j = 0 ; j < surf->numVerts * surf->numFrames ; j++, xyz++ ) 
		{
            xyz->xyz[0] = LittleShort( xyz->xyz[0] );
            xyz->xyz[1] = LittleShort( xyz->xyz[1] );
            xyz->xyz[2] = LittleShort( xyz->xyz[2] );

            xyz->normal = LittleShort( xyz->normal );
        }


		// find the next surface
		surf = (md3Surface_t *)( (byte *)surf + surf->ofsEnd );
	}
    
	return qtrue;
}



qhandle_t R_RegisterMD3(const char* name, model_t* mod)
{

	char* buf;
	int	numLoaded = 0;

	ri.FS_ReadFile( name, (void**)&buf );
	
    if( NULL != buf)
    {
        qboolean loaded = qfalse;

#if defined( Q3_BIG_ENDIAN )		
		int ident = LittleLong(*(int *)buf);
#else
		int ident = *(int *)buf;
#endif
		if (ident == MD3_IDENT)
			loaded = R_LoadMD3(mod, 0, buf, name);
		else
			ri.Printf(PRINT_WARNING,"R_RegisterMD3: unknown fileid for %s\n", name);
		
        ri.FS_FreeFile(buf);

        if(loaded)
		{
			mod->numLods++;
			numLoaded++;
		}
        else
        {
            ri.Printf(PRINT_WARNING, "R_RegisterMD3: couldn't load %s\n", name);

	        mod->type = MOD_BAD;
	        return 0;
        }
    }
    else
    {
        ri.Printf(PRINT_WARNING, "R_RegisterMD3: failed loading %s from disk. \n", name);
    }


	char filename[MAX_QPATH] = {0};
	strcpy(filename, name);
	char* const dot = strrchr(filename, '.');
    *dot = 0;

    uint32_t lod;
	for (lod = 1; lod < MD3_MAX_LODS; lod++)
	{
        qboolean	loaded = qfalse;

		char namebuf[MAX_QPATH+20] = {0};
        snprintf(namebuf, sizeof(namebuf), "%s_%d.md3", filename, lod);

		ri.FS_ReadFile( namebuf, (void**)&buf );
		if(!buf)
			continue;
		
#if defined( Q3_BIG_ENDIAN )		
		int ident = LittleLong(*(int *)buf);
#else
		int ident = *(int *)buf;
#endif
		if (ident == MD3_IDENT)
			loaded = R_LoadMD3(mod, lod, buf, name);
		else
			ri.Printf(PRINT_WARNING, "R_RegisterMD3: unknown fileid for %s\n", name);
		
		ri.FS_FreeFile(buf);

		if(loaded)
		{
			mod->numLods++;
			numLoaded++;
		}
		else
			break;
	}

	if(numLoaded)
	    return mod->index;
    else
        ri.Printf(PRINT_WARNING, "R_RegisterMD3: couldn't load %s\n", name);


/*
	if(numLoaded)
	{
		// duplicate into higher lod spots that weren't loaded,
        // in case the user changes r_lodbias on the fly
		for(lod--; lod >= 0; lod--)
		{
			mod->numLods++;
			mod->md3[lod] = mod->md3[lod + 1];
		}

		return mod->index;
	}
*/

	mod->type = MOD_BAD;
	return 0;
}
