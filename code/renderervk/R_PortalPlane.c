#include "R_PortalPlane.h"

static struct rplane_s g_portalPlane;		// clip anything behind this if mirroring

inline static float DotProduct( const float v1[3], const float v2[3] )
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}


void R_SetupPortalPlane(const float axis[3][3], const float origin[3])
{
	// VectorSubtract( vec3_origin, pCamera->axis[0], g_portalPlane.normal );
    // g_portalPlane.dist = DotProduct( pCamera->origin, g_portalPlane.normal );

    g_portalPlane.normal[0] = - axis[0][0];
    g_portalPlane.normal[1] = - axis[0][1];
    g_portalPlane.normal[2] = - axis[0][2];
    g_portalPlane.dist = - origin[0] * axis[0][0]
                         - origin[1] * axis[0][1]
                         - origin[2] * axis[0][2];
}


void R_TransformPlane(const float R[3][3], const float T[3], struct rplane_s* pDstPlane)
{
    pDstPlane->normal[0] = DotProduct (R[0], g_portalPlane.normal);
    pDstPlane->normal[1] = DotProduct (R[1], g_portalPlane.normal);
    pDstPlane->normal[2] = DotProduct (R[2], g_portalPlane.normal);
    pDstPlane->dist = DotProduct (T, g_portalPlane.normal) - g_portalPlane.dist;
}
