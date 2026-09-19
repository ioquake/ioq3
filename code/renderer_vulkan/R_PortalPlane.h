#ifndef R_PORTAL_PLANE_H_
#define R_PORTAL_PLANE_H_


struct rplane_s {
	float normal[3];
	float dist;
};

// extern struct rplane_s g_portalPlane;		// clip anything behind this if mirroring


void R_SetupPortalPlane(const float axis[3][3], const float origin[3]);
void R_TransformPlane(const float R[3][3], const float T[3], struct rplane_s* pDstPlane);

#endif
