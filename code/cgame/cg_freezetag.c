#include "cg_local.h"

qboolean CG_FreezeTag(void) {
	return cgs.gametype == GT_FREEZETAG;
}

qboolean FT_LocalIsFrozen(void) {
	return cg.snap->ps.powerups[PW_FREEZE];
}

qboolean FT_PlayerIsFrozen(centity_t *cent) {
	return cent->currentState.powerups & (1 << PW_FREEZE);
}
