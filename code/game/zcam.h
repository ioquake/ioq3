/* 
 *  ZCam v1.0.4
 *  Spectator Camera for Quake III Arena
 *  Copyright (C), 2001 by Avi "Zung!" Rozen
 *	
 *  http://www.telefragged.com/zungbang/zcam
 *
 *  Credits:
 *  FLIC  camera mode is based on code  taken from q2cam by Paul Jordan
 *  SWING camera mode is based on ideas taken from CreepCam for Quake I 
 *
 */

#ifndef __ZCAM_H__
#define __ZCAM_H__

#define MAX_VISIBLE_RANGE		1000

void camera_init(void);
void camera_shutdown(void);
void camera_state_save(gclient_t * client);
void camera_state_load(gclient_t * client);
void camera_begin(gentity_t * ent);
void camera_disconnect(gentity_t * ent);
void camera_think(gentity_t * ent);
void camera_cmd(gentity_t * ent);

#endif				/* __ZCAM_H__ */
