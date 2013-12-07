Z-Camera v1.0.4
Spectator Camera for Quake III Arena
Copyright (C), 2001 by Avi "Zung!" Rozen

http://www.telefragged.com/zungbang/zcam

About
=====
Z-Camera is a mod for Quake III Arena that replaces the standard
spectator mode with an automatic camera. 

The spectator can select between two camera modes: 
* FLIC mode: the camera films the most interesting players and areas
  at any given moment.
* SWING mode: the camera swings around a specific player, selected by
  the spectator, while tracking the most interesting opponent.

 
Installation
============
Extract this archive to the base Quake III Arena folder.
This should create a new sub-folder named 'zcam'.

Usage
=====
Use '/team spectator' or the Q3 start menu to switch to spectator mode.
The default camera mode is FLIC. 
Use ENTER (+button2) to switch between FLIC and SWING modes.
Use CTRL (+attack) to cycle target players in SWING mode.
Use "/camera prev" and "/camera next" to switch to the previous
or next player (cycle). 
Use "/camera flic" and "/camera swing" to select camera mode.

The original spectator follow mode is still available. Just use the
follow/follownext/followprev commands.

Enjoy the show!

Credits
=======
FLIC  camera mode is based on code  taken from q2cam by Paul Jordan
SWING camera mode is based on ideas taken from CreepCam for Quake I 


History
=======
28 Dec. 2001 - Version 1.0.4
	       * added: console commands "camera swing" and 
	         "camera flic" for selecting camera mode 
	       * added: camera mode is displayed when changing modes
	       * modified: camera stays in SWING mode when level
	         changes (used to fall back to FLIC mode) 
	       * modified: camera stays in SWING mode when a chased
	         player disconnects, and moves to next player (used to
	         fall back to FLIC mode) 
	       * fixed bug: server crashed upon switching to spectator
	         mode, with no players connected (usually in team play
	         modes)	
20 Nov. 2001 - Version 1.0.3
	       * modified: separated target selection code from the
	         rest, to simplify porting the camera to other mods
	       * modified: changed counting of visible players to
	         ignore teams
19 Nov. 2001 - Version 1.0.2
	       * added: console command "camera"
	         "camera prev" switches to previous client
	         "camera next" switches to next client
	       * added: message is shown when swing camera switches
	         targets
	       * fixed bug: swing camera was very erratic at close
	         range 
16 Nov. 2001 - Version 1.0.1
	       * fixed bug: swing camera "jumped" when target died
11 Nov. 2001 - Version 1.0.0
	       * fixed bug: camera jitter eliminated
