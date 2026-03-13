#include "tr_local.h"
#include "tr_model.h"


void RE_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs )
{
	model_t* model = R_GetModelByHandle( handle );

	if(model->type == MOD_BRUSH)
    {
		VectorCopy( model->bmodel->bounds[0], mins );
		VectorCopy( model->bmodel->bounds[1], maxs );
		return;
	}
    else if (model->type == MOD_MESH)
    {
		md3Header_t* header = model->md3[0];
		md3Frame_t* frame = (md3Frame_t *) ((unsigned char *)header + header->ofsFrames);

		VectorCopy( frame->bounds[0], mins );
		VectorCopy( frame->bounds[1], maxs );
		return;
	}
    else if (model->type == MOD_MDR)
    {
		mdrHeader_t* header = (mdrHeader_t *)model->modelData;
		mdrFrame_t* frame = (mdrFrame_t *) ((unsigned char *)header + header->ofsFrames);

		VectorCopy( frame->bounds[0], mins );
		VectorCopy( frame->bounds[1], maxs );
		
		return;
	}
    else if(model->type == MOD_IQM)
    {
		iqmData_t* iqmData = model->modelData;

		if(iqmData->bounds)
		{
			VectorCopy(iqmData->bounds, mins);
			VectorCopy(iqmData->bounds + 3, maxs);
			return;
		}
	}

	VectorClear( mins );
	VectorClear( maxs );
}
