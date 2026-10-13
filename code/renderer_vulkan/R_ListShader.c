#include "tr_globals.h"
#include "tr_shader.h"
#include "ref_import.h"

/*
===============
Dump information on all valid shaders to the console
A second parameter will cause it to print in sorted order
===============
*/
void R_ShaderList_f (void)
{
	int			i;
	int			count = 0;
	shader_t*   pShader;

	ri.Printf (PRINT_ALL, "-----------------------\n");

	for ( i = 0 ; i < tr.numShaders ; i++ )
    {
		if ( ri.Cmd_Argc() > 1 )
			pShader = tr.sortedShaders[i];
		else
			pShader = tr.shaders[i];

		ri.Printf( PRINT_ALL, "%i ", pShader->numUnfoggedPasses );

		if (pShader->lightmapIndex >= 0 )
			ri.Printf (PRINT_ALL, "L ");
		else
			ri.Printf (PRINT_ALL, "  ");


		if ( pShader->multitextureEnv == GL_ADD )
			ri.Printf( PRINT_ALL, "MT(a) " );
		else if ( pShader->multitextureEnv == GL_MODULATE )
			ri.Printf( PRINT_ALL, "MT(m) " );
		else
			ri.Printf( PRINT_ALL, "      " );
	

		if ( pShader->explicitlyDefined )
			ri.Printf( PRINT_ALL, "E " );
		else
			ri.Printf( PRINT_ALL, "  " );


		if ( !pShader->isSky )
			ri.Printf( PRINT_ALL, "gen " );
		else
			ri.Printf( PRINT_ALL, "sky " );


		if ( pShader->defaultShader )
			ri.Printf (PRINT_ALL,  ": %s (DEFAULTED)\n", pShader->name);
		else
			ri.Printf (PRINT_ALL,  ": %s\n", pShader->name);

		count++;
	}
	ri.Printf (PRINT_ALL, "%i total shaders\n", count);
	ri.Printf (PRINT_ALL, "------------------\n");
}

