#include "tr_local.h"
#include "tr_model.h"
#include "ref_import.h"

static md3Tag_t *R_GetTag( md3Header_t *mod, int frame, const char *tagName ) {
	md3Tag_t		*tag;
	int				i;

	if ( frame >= mod->numFrames ) {
		// it is possible to have a bad frame while changing models, so don't error
		frame = mod->numFrames - 1;
	}

	tag = (md3Tag_t *)((byte *)mod + mod->ofsTags) + frame * mod->numTags;
	for ( i = 0 ; i < mod->numTags ; i++, tag++ ) {
		if ( !strcmp( tag->name, tagName ) ) {
			return tag;	// found it
		}
	}

	return NULL;
}

static md3Tag_t *R_GetAnimTag( mdrHeader_t *mod, int framenum, const char *tagName, md3Tag_t * dest) 
{
	int				i, j, k;
	int				frameSize;
	mdrFrame_t		*frame;
	mdrTag_t		*tag;

	if ( framenum >= mod->numFrames ) 
	{
		// it is possible to have a bad frame while changing models, so don't error
		framenum = mod->numFrames - 1;
	}

	tag = (mdrTag_t *)((byte *)mod + mod->ofsTags);
	for ( i = 0 ; i < mod->numTags ; i++, tag++ )
	{
		if ( !strcmp( tag->name, tagName ) )
		{
			Q_strncpyz(dest->name, tag->name, sizeof(dest->name));

			// uncompressed model...
			//
			frameSize = (intptr_t)( &((mdrFrame_t *)0)->bones[ mod->numBones ] );
			frame = (mdrFrame_t *)((byte *)mod + mod->ofsFrames + framenum * frameSize );

			for (j = 0; j < 3; j++)
			{
				for (k = 0; k < 3; k++)
					dest->axis[j][k]=frame->bones[tag->boneIndex].matrix[k][j];
			}

			dest->origin[0]=frame->bones[tag->boneIndex].matrix[0][3];
			dest->origin[1]=frame->bones[tag->boneIndex].matrix[1][3];
			dest->origin[2]=frame->bones[tag->boneIndex].matrix[2][3];				

			return dest;
		}
	}

	return NULL;
}

static void Matrix34Multiply_OnlySetOrigin( float *a, float *b, float *out ) {
	out[ 3] = a[0] * b[3] + a[1] * b[7] + a[ 2] * b[11] + a[ 3];
	out[ 7] = a[4] * b[3] + a[5] * b[7] + a[ 6] * b[11] + a[ 7];
	out[11] = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11];
}



static void ComputeJointMats( iqmData_t *data, int frame, int oldframe,
			      float backlerp, float *mat ) {
	float	*mat1;
	int	i;

	ComputePoseMats( data, frame, oldframe, backlerp, mat );

	for( i = 0; i < data->num_joints; i++ ) {
		float outmat[12];
		mat1 = mat + 12 * i;

		memcpy(outmat, mat1, sizeof(outmat));

		Matrix34Multiply_OnlySetOrigin( outmat, data->jointMats + 12 * i, mat1 );
	}
}


static int R_IQMLerpTag( orientation_t *tag, iqmData_t *data,
		  int startFrame, int endFrame, 
		  float frac, const char *tagName ) {
	float	jointMats[IQM_MAX_JOINTS * 12];
	int	joint;
	char	*names = data->names;

	// get joint number by reading the joint names
	for( joint = 0; joint < data->num_joints; joint++ ) {
		if( !strcmp( tagName, names ) )
			break;
		names += strlen( names ) + 1;
	}
	if( joint >= data->num_joints ) {
        memset(tag, 0, sizeof(orientation_t));

		return qfalse;
	}

	ComputeJointMats( data, startFrame, endFrame, frac, jointMats );

	tag->axis[0][0] = jointMats[12 * joint + 0];
	tag->axis[1][0] = jointMats[12 * joint + 1];
	tag->axis[2][0] = jointMats[12 * joint + 2];
	tag->origin[0] = jointMats[12 * joint + 3];
	tag->axis[0][1] = jointMats[12 * joint + 4];
	tag->axis[1][1] = jointMats[12 * joint + 5];
	tag->axis[2][1] = jointMats[12 * joint + 6];
	tag->origin[1] = jointMats[12 * joint + 7];
	tag->axis[0][2] = jointMats[12 * joint + 8];
	tag->axis[1][2] = jointMats[12 * joint + 9];
	tag->axis[2][2] = jointMats[12 * joint + 10];
	tag->origin[2] = jointMats[12 * joint + 11];

	return qtrue;
}


int RE_LerpTag( orientation_t *tag, qhandle_t handle, int startFrame, int endFrame, 
					 float frac, const char *tagName )
{

    // ri.Printf(PRINT_ALL, "R_LerpTag\n");

	md3Tag_t	*start, *end;
	md3Tag_t	start_space, end_space;
	model_t* model = R_GetModelByHandle( handle );
	if ( !model->md3[0] )
	{
		if(model->type == MOD_MDR)
		{
			start = R_GetAnimTag((mdrHeader_t *) model->modelData, startFrame, tagName, &start_space);
			end = R_GetAnimTag((mdrHeader_t *) model->modelData, endFrame, tagName, &end_space);
		}
		else if( model->type == MOD_IQM ) {
			return R_IQMLerpTag( tag, model->modelData,
					startFrame, endFrame,
					frac, tagName );
		} else {
			start = end = NULL;
		}
	}
	else
	{
		start = R_GetTag( model->md3[0], startFrame, tagName );
		end = R_GetTag( model->md3[0], endFrame, tagName );
	}

	if ( !start || !end ) {
        memset(tag, 0, sizeof(orientation_t));
		return qfalse;
	}

    
	uint32_t i;
    for ( i = 0 ; i < 3 ; i++ ) {
		tag->origin[i] = start->origin[i] + (end->origin[i] - start->origin[i]) * frac;
		tag->axis[0][i] = start->axis[0][i] + (end->axis[0][i] - start->axis[0][i]) * frac;
		tag->axis[1][i] = start->axis[1][i] + (end->axis[1][i] - start->axis[1][i]) * frac;
		tag->axis[2][i] = start->axis[2][i] + (end->axis[2][i] - start->axis[2][i]) * frac;
	}
	VectorNormalize( tag->axis[0] );
	VectorNormalize( tag->axis[1] );
	VectorNormalize( tag->axis[2] );
	return qtrue;
}
