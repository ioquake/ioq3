
Server-side demos patch (entities/events oriented) for ioquake3
===============================================================================================


DESCRIPTION
-----------
Fully working server-side demos for ioquake3 are now a reality!

This patch provide a full server-side demos facility for ioquake3 github commit of 2017-03-04. For the original patch for OpenArena with full commit history, see https://github.com/lrq3000/openarena_engine_serversidedemos .

This patch was done by Stephen Larroque and is based on the original patch by Amanieu d'Antras for Tremfusion (Tremulous).

The approach used here is entity/event oriented demo recording, which means that each event and entity change is recorded in the demo. At playback, the whole game state is replayed every frame. In other words, a server-side demo is a simulation of a real game, with "ghost" entities and players.

This implementation has been made as generic as possible, and so it should work for any mod based on ioquake3 or Quake 3 Arena. It was also cleaned up and separated as much as possible from the core code, leaving a minimum of changes to the core files, so it should be easily portable to any version of ioquake3 >= r1910 (maybe also with a few prior versions).

NOTE: this is a port from OpenArena v0.8.8 to OA+ioquake3, this should be even more close to the original ioquake3 code.

NOTE2: an alternative is to record multiview demos (which is not what this patch does), but server-side. A multiview demo records each players stream (= network snapshot packets), and just replays them. This is even more generic than this patch's approach, as it should work with virtually any mod and any configuration possible (since it would be agnostic to the network snapshots content, it would just replay them!). If you are interested by this approach, see TheDoctor's patch (see github releases/TheDoctor-serverside-demo_v0.4.patch.zip) or better the [eDawn patch](http://edawn-mod.org/forum/viewtopic.php?f=5&t=7) ([more info here](http://edawn-mod.org/binaries/quake3e-mv.txt)) (see github releases/q3e-multiview-patch-edawn.zip). This approach could be extended to provide a full replacement to GTV (ie, to rebroadcast matchs in realtime using a man-in-the-middle server).


FEATURES
--------

* Allows to record full server-side demos
* Can autorecord, with meaningful automatically generated filename (format: hostname-date-time-map.sv_dmxx)
* Can record demos in mods
* Privacy checking: filters out privacy data (not even recorded in the demo file)
* Save meta-data of the demo (infos about the demo, like the UTC datetime)
* Automatically switch the correct gametype/mod/map/limits when replaying a demo
* Can play demos on a patched server, clients can connect and see server-side demos replays without needing a patched client.
* Can play demos locally using a patched client (no need to setup a server).
* Can be used as an alternative to GTV by rebroadcasting a demo (can be done in realtime as the demo is being written)
* `timescale` and `cl_freezeDemo` are supported to slowdown/speedup/pause a demo (both locally on a client, or on a server replay).


INSTALL
-------

Simply compile the code into a binary, and use these binaries. You do not need to add any .pk3 nor make your server unpure, this will be totally transparent.

Servers obviously need these binaries to record and play demos.

Clients also need these binaries to play demos locally, unless they connect to a server replaying the demos, in this case they don't need anything.

Indeed, a server issuing `/demo_play <filename>` will be accessible to spectators. This can be useful to replay a big event. This can also be used as a replacement to GTV, since you can read a demo at the same time as it is being written.
USAGE
-----

Commands:

* `demo_play <filename>` : playback a server-side demo (to be found, the demo must be in the current mod folder, even if it was recorded with another mod). Note that clients need to go back to the main menu before issuing the `/demo_play <filename>` command, else the demo won't be found. The demo will automatically switch mods if necessary, and load the correct map.
* `demo_record <filename>` : record a server-side demo with the given filename (will be saved in mod/svdemos folder). For automated demo recording, see sv_autoDemo cvar below.
* `demo_stop` : stop any playback/recording (will automatically restore any previous setting on the server/client). Note that shutting down the server/quitting the game will not break the demo, the demo will still be readable.
* `status` : as with normal clients, when a demo is replaying, democlients will also be shown in the status (with ping DEMO).

Special cvars:

* `sv_autoDemo 1` : enable automatic recording of server-side demos (will start at the next map change/map_restart).
* `sv_demoTolerant 1` : enable demo playback compatibility mode. If you have an old server-side demo, or a bit broken, this can maybe allow you to playback this demo nevertheless.
* `sv_democlients` : show number of democlients (automatically managed, this is a read-only cvar).
* `sv_demoState` : show the current demo state (0: none, 1: waiting to play a demo, 2: demo playback, 3: waiting to stop a demo, 4: demo recording).



UPDATE ON GIT
-------------

Just like the openarena_engine repo, this one was rebased onto the latest openarena_engine repo (ioquake3+OA088), so that all changes from ioquake3 to openarena are on top of the latest ioquake3 changes, and all the changes from openarena to this sv_demo patch are on top of the openarena commits. Thus, you have a linear commit graph which is easy to rebase against new versions, and the history is more human readable.

To update this repo, you can:

    # First clone the git repo locally on your computer
    git clone git://github.com/lrq3000/openarena_engine_serversidedemos.git
    
    # Then either sync with openarena_engine
    git remote add upstream git://github.com/lrq3000/openarena_engine.git
    
    # Or directly with ioquake3
    # git://github.com/ioquake/ioq3.git
    
    AUTO-PROCEDURE
    ##############
    # Then try to pull + rebase the changes automatically
    git pull --rebase upstream master
    
    # If it works, then all is done! You have an updated version on your computer. You can stop here and just compile the engine using the tutorials in the ioq3 wiki: http://wiki.ioquake3.org/Building_ioquake3
    # PS: for Windows, use the cygwin tutorial, don't try mingw unless you are very experienced and have got a lot of spare time to mess with the weird errors you will get.
    
    MANUAL PROCEDURE
    ################
    # Else, you have to redo the process but manually this time. Following steps:
    
    # First, you have to reinitialize you repo, else it won't accept you do anything:
    $ git rebase --abort
    $ git reset --hard origin/master
    
    # Git rebase interactive so that you get the list of the last commits. This will open you a text file in your default text editor program. What you have to do here is to cut all the lines that concerns commits about the sv_demo patch. Then paste these lines in a temporary text file for you (we will later use the SHA code of the commmits).
    git rebase HEAD~10 --interactive
    
    # Now you should have a clean repo without the sv_demo commits, only an old revision of the openarena_engine.
    
    # We will now sync to the latest changes of openarena_engine or ioquake3:
    git pull -s recursive -X theirs upstream master
    # This will pull all remote changes and accept them automatically.
    
    # Now, you should have a clean repo synced to the latest revision of the engine. We just have now to reapply the sv_demo commits one by one.
    
    # Reopen the text file where you pasted those sv_demo commits lines before (when we did rebase --interactive), and follow the commits in the natural order: from the top to the bottom. The top commit being the oldest one, and the lowest the latest.
    # You have to pick the oldest commit first, then to the more recent and so on. Look at the SHA code (the second column after the command "pick"), and copy this SHA code below in place of 617a:
    git cherry-pick 617a
    # This command will pick this very specific commit and will try to reapply it.
    # It will try to automatically merge the conflicts.
    
    # In the case there are some unresolved conflicts, you can resolve them manually by doing:
    git mergetool
    
    # If you resolved manually the conflicts, then you have to do:
    git commit
    
    # And then, you can continue onto the next sv_demo commit:
    git cherry-pick ...
    
    # Until you reapplied all the commits, and voila, you have an up-to-date engine with the sv_demo patch!


DEV NOTES
---------

* In msg.c: if ( cl_shownet && ...  IS necessary for the patch to work, else without this consistency check the engine will crash when trying to replay a demo on a server (but it will still work on a client!)
  NOTE: This was merged in a patch in the ioquake3 project, and this fix is now officially part of the engine.

* usercmd_t management (players movement commands simulation) is implemented but commented out. It fully works, but it's not necessary for the demo functionnalities, and it adds a LOT of data to the demo file, so demo files take a lot more harddrive space when this function is enabled. If you want to do demo analysis, it is advised to turn on this feature, else you should probably not.

* The patch architecture is pretty simple: we record every events/entities/playerStates at demo recording, and for playback we just hook at the end of each server frame and overwrite with demo events. This way, demo events always take the upper hand on server's events, but it still allows the server to manage interpolation when there is no demo event. This is why the timescale and cl_freezeDemo functions work with server-side demos.



TODO
----

* Fix demo filenames: legacy protocol cvar? client side demos have extension dm_66 on ioq3 but svdemos have dm_71

* ExcessivePlus new scoreboard is buggy and shows wrong scores and stats, except when one of the players die (producing and sending a new scoreboard state, so this forces the gamecode to update with the correct scoreboard state from demo). A generic solution might be to: always save the full server/gamecode state at the end of one frame (in sv_demo.c), and then at the beginning of next frame restore the full server/gamecode state. This way, each demo frame would pick up right from the last demo state, guaranteed. Because here the scoreboard bug is probably due to ExcessivePlus having a too high scoreboard refresh rate, so it gets refreshed between demo frames where there is a scoreboard refresh (in other words, E+ is refreshing the scoreboard even though there is no new info: E+ is active in its approach to scoreboard refreshing, whereas ioq3 is passive and waits for real updates). But with this approach, check if timescale and cl_freezeDemo still work (because saving/restoring full server state might break ability to interpolate frames between recorded demo frames). Might want to look into RestoreCmdContext() and SaveCmdContext().


SHOULD DO (but not now)
-----------------------

* please wait before switching teams should not be printed (but it's a standard gameCommand, fixing it would be very unelegant and add a lot of complexity to the code for such a special case - or maybe just move SV_GameSendServerCommand() hook into clientNum == -1 only? Wouldn't that prevent the recording of some other important command strings?)

* Delagsimulation when replaying a demo to see in the "eye of the beholder". Probably should be done as a gamecode modification, either at recording by storing the client-side world state after delag, or by simulating the delag at replaying from demo and pings infos (already recorded normally).

* Fix usercmds_t replaying (by fixing command time, I think it's not set correctly and so the commands are dropped), see g_active.c ClientThink_real():
  msec = ucmd->serverTime - client->ps.commandTime;
  // following others may result in bad times, but we still want
  // to check for follow toggles
  if ( msec < 1 && client->sess.spectatorState != SPECTATOR_FOLLOW ) {
  	return;
  }

* When demo replaying a demo client-side with mod switching, sv_cheats is disabled (prevent timescale and other commands to be used)

* Fix tournament when nb players < 2: if a demo is recorded before demo players connected, real players connecting before demo players joined will join automatically. The only way to fix in ExcessivePlus that is to manually enter /speconly then /team s. For vanilla ioquake3, wait for demoplayers to connect and then go to spectators. This is not critical to fix anymore since these "buggy" demos (where real players will be forced to join the demo game) are recorded in a separate "warmup" demo, and the real match that starts after when two players connect are recorded in another demo. So the match demo has no issue at all (because there are two democlients, so the real players cannot join anymore), only the warmup demo can still allow real players to join the game (but they are a lot less interesting anyway with only one democlient!).


KNOWN BUGS (WONT FIX FOR NOW)
-----------------------------
Below is a list of known bugs or wished features, but if you encounter them, please report anyway. If a bug is reported to be too hampering, it may get fixed in the future.

* Demo crashing with error "server disconnected - server command overflow". If you get this error when they try to replay, it means your graphical settings are too slow for your computer. Try using a lower resolution or change graphical parameters to solve your graphical slowness issues, this should solve the demo playback. Indeed, demo playback needs to be realtime, so if your computer is slow, the demo will crash.

* save the minimum correct value for sv_democlients when recording: count the total number of clients (>= CS_ZOMBIE) per frame, and the highest number count will be the good number (or just look at the highest clientid reached since client slots are filled in ascending order).

* entityShared_t, entityState_t and playerState_t could be normalized with the other functions to put in write functions and use a marker per entity instead of a marker for a whole lot of entities (but maybe this would require more space? but would maybe be better to read the demo, more coherent: one marker, one event). Because for now, these are the only functions that write ALL data for ALL entities at once, instead of one entity per call, and thus, these functions are managed in a special way compared to others.

* team0 bug at demo start/end: when the server change sv_democlients and sv_maxclients, some data aren't copied over, or the gamecode is not notified of the change. Anyway, all my tries to fix that broke completely the engine (see SV_ChangeMaxClients() in sv_init.c if you want to give it a try). WORKAROUND: now the patch automatically force real clients to spectator, so this should not be an issue anymore (and in fact it happens when the gamecode thinks it's not a team-based gametype, so it makes the clients auto join in, but it's weird that sometimes it does the same thing when its >= GT_TEAM !).

* NOT POSSIBLE: save all client_t (and clientState_t), player_t (and playerState_t), and gentity_t (sv.gentities) fields (and subfields) in demos. Advantage: theoretically 100% faitful demo. Cons: a big space hog and some fields should NOT be saved or they will cause a weird behaviour of the engine (such as netchan or download management fields).
  the best would be a polymorphic recursive function that would automatically read the specification of the object given or subobjects and automatically create the good fields, and when reading back the recording it would automatically know how to read the data based on the specification too).
  currently: only gentity_t->entityShared_t and gentity_t->entityState_t and playerState_t are recorded. Other fields (except health and speed) are NOT recorded (eg: gclient_s *client, gitem_t *item, etc..).
  Please note that we already save a maximum of data, in fact all the data that will ever be needed. But this is not generic (we pick each info we want), maybe it would be better to have a generic save function for the whole data structure, easily adaptable to any game that adds more data fields?

* SendConsoleCommand save in demos (will record postgame data and teamtask) G_SEND_CONSOLE_COMMAND and reproduce with Cbuf_ExecuteText( args[1], VMA(2) ); - not a good idea because there are map_restart commands that may be catched, and we don't want that (and without this hook, the patch really works pretty well).

* Prevent all recorded sv_game.c commands to be accepted when demo_playback? Bad idea I think.

* Store and replay network messages, similarly to TheDoctor's patch, but redirect them to all the real players who are spectating the client? Could be nice, but hard to combine with the current infrastructure. And in fact, replaying demomessages can't replay all events when free flying. NO: in fact this method should be implemented in a separate patch and could lead to a full GTV-like alternative (and also to low CPU server-side demos).

* scoreboard is not ordered in descending order of score (except when a player dies, it forces the engine to refresh with the demo's infos) - problem linked to the same cause that produces the ping problem.

* Sometimes (not that it's not _always_, if it happens _always_ then that's another bug you should report) print or cp messages are issued more than once. This is a bug caused by the fact that the engine automatically issue messages after some events (such as ClientBegin), but they are also recorded in the demo file. When replaying, there's a check function that tries to avoid duplicates, but sometimes a few cases will slip in: to be more precise, when a game command from the demo file is issued before the event happening in the demo triggers the engine to issue a game command. The other way around (engine game command then demo game command) is already handled, but the other way around not, because we don't want to prevent the engine to issue its messages in any way.

* clients cid is recorded on a Byte, so it supports only a value between 0 and 255 (can easily change that to long if necessary).

* When loading a demo on a server that was recorded on another mod than the one currently loaded, when the server will switch automatically the mod, it will disconnect all connected players with the message "Game Directory Changed". This is normal and unavoidable (except if you find a genius way to keep the server running while hotswapping the mod, then submit your patch to ioquake3).

* Non-player entities health is NOT recorded. It could be, now that a get function was done, but it would complexify the code and I'm not sure if this would really benefit something. When entities run out of health, they are anyway destroyed (since their state is recorded), but their health not. This means that the health that is shown in the crosshair when aiming at an entity will not be updated in a demo.

* set_cvar too? and at startup just like configstrings (will avoid timelimit)? Bad idea too.

* Cvars changed in the demo aren't set when replayed, but if they affect an aspect of the gameplay (such as changing g_gravity), it will be reflected in the demo because the whole entities states are recorded, so it will be faithfully reproduced even if the cvars aren't set (if you have a demo where that's not the case, please post it and describe).

* ExcessivePlus: when replaying a demo, democlients whose initial team was spectator can be spectated (but subsequent team change will make them unspectatable if they go to spec).



CHANGELOG (newest to the bottom)
--------------------------------

* Map not issued after game_restart, either fix or delay a bit
  because after game_restart need to change again sv_democlients and sv_maxclients (add a delay if that doesn't work directly with cbuf_addtext)
* excessiveplus map_restart nonstop (because of vars checking!) - no because of gamecommand setting system reserved configstrings.
* demo messages are too much repeated with excessiveplus. This is because of GameCommands, which are repeated per client connected (weird behaviour, normally only commands broadcasted to everybody, so just sent once, are recorded, so E+ sends multiple times the same command when it shouldn't).
* bots away bug?
* demostop, when from baseoa to excessiveplus demo, then play again and it crashes (was just the last savedFsGame affectation that wasn't right, was not using strcpy).
* bots team joining bug: maybe the strcpy is not right? (see the thing that happened with fs_game - maybe userinfo? when it will be fixed?). SOLUTION: was just g_doWarmup, nothing to do with bots in fact, it just waited for enough players to be playing (and democlients ARE considered to be playing) to start the warmup, without announcing it.
* autorecord doesn't work anymore. look at the log and try old versions. Probably there's a command that is recorded and that shouldn't at the beginning of a map. Solution: forgot to move the check to avoid recording system configstrings.
* Filtering sv_hostname to disallow bad characters such as ":" on Windows OSes. It will still record a demo, but everything behind this name will be dropped. That's why the hostname is set last, to workaround this problem meanwhile.
* demo is already replaying... add message: use demo_stop to stop any recording/replaying and retry
* timelimit, fraglimit, capturelimit store and replay too?
* health gamecode update (set a g_demoPlaying var and from the server I can Cvar_SetValue very easily).
* forceteam spec only if player is connected
* move these writeframe if to functions
* move switch readframe to functions
* auto sv_democlients 0 at startup
* fix writestring warning for configstrings index
* messages again repeated...
* broadcast message cp when demo starts
* g_autoDemo auto disable on demo launching after a certain point (after corruptions and such, just before a map_restart)
* save hostname and restore it after? No because player won't know where they are. We save it, but that's all (can be printed as a demo info like: Demo infos: demo was recorded on server ... at date-time, map, gametype, sv_fps and ...)
* Can't replay a demo when launched by client launcher (can't find the file???)
* team bug set (see current qconsole10.log)
* special variable sv_demoTolerant 1 to enable faults tolerance mode (will be tolerant to parsing errors: 1- pass if illegible message error when reading frames 2- when reading a demo headers, new format: string for the variable name to set, then value, and in a if until string "END" continue to read and set all cvars. This means that by enabling this mode, you can probably read all demos recorded from this version up to any version. So in the future, you will still be able to replay demos recorded with previous versions of this patch (if version >= v0.9.4.3). Concretely, with sv_demoTolerant 1, new demo messages and missing new meta data (meta data implemented later) will just be skipped.
* events messages support for coloured strings (such as coloured names)
* what happens if a cvar is changed during the demo such as gravity? It's ok, if it affects the gameplay, it's reproduced in the demo (and if it does not, then we don't have to care anyway).
* g_gametype not necessarily set back if game_restart (latched but needs another map_restart)
* normal client demo e+ bug -> due to bad refreshing of democlients and svmaxclients (svmaxclients is not refreshed after game_restart). This was because of game_restart mod switch, sv_maxclients nor sv_democlients was set at the right time, so another delay command was issued, which cumulated with previously issued delay commands, and at the end it gave an infinite loop of map_restart (since all delay would trigger non-stop).
* MAJOR BUG: sv_autoDemo 1 and real clients disconnected because of wrong guid at each map change. Was because when filtering userinfo string, it rewritten over the client userinfo string instead of copying it over to another var (so the guid and ip was removed from the client's infos! And the server dropped the clients at the next time they tried to reconnect, such as map change).
* support timescale change when replaying a demo (still a bit buggy with very low values or very high, but it works)
* support for cl_freezeDemo to freeze the demo (works but the camera can't move when in spectator! That's normal since we block the time and so it blocks any movement. This is in fact the same behaviour as in standard demos.)
* sometimes the name is not refreshed, maybe client_t name or netname field is not updated?
* Demo recorded during the warmup ARE buggy (most of the time can't even be replayed!). This is normal (because of the warmup, produces a lot of weird issues), so maybe just detect when it's warmup time and disable auto recording?
  was because of demo initial time that was too small (400) and sv.time too high, even if we just have restarted the map (at least 440 in practice...), so the solution was to let some demo frames replay in the void, so that we can artificially adjust the demo to the current sv.time and then play normally.
* SHA demo stops after a few seconds? lol why?
* dm not working... like SHA, stops after a few seconds
* tourney: demo file is corrupted or quit like dm
* tdm like dm
* overload infinite loop like warmup
* if tourney mode: should NOT issue /team because it will set the player at the end of the list (and cannot be spectated!)
* overload entities bug?
* NET_CompareBaseAdr: bad address type error?
* new players at connection in DM automatically goes to the game
* free malloc'ed strings
* strpy -> Q_strncpyz
* test with other mods
* warmup still recording problem with demo and excessiveplus... - yes but no problem, the demo continues to play
* fix issues with very low timescale (the game speeds up! the rounding in the calculation must be producing a big rounding error somewhere).
* Generations Arena: team management does not work (when a player switch team, he is not affected to the right team)
* E+ now real players join the game...
* E+ democlients are not spectatable, userinfo is not reliable - now update, specs are spectatable...
* cleaned FIXME
* clean code
* Fix inactivity timers (simulate UserMove or just send a fake usercmd_t) - had to craft a remoteAddress with NET_StringToAdr, because else if we just use Info_SetValueForKey(userinfo, "ip", "localhost") the server would remove the ip key in the userinfo because no real address can be found for democlients.
* SV_DemoChangeMaxClients() does not consider privateclients reserved slots when moving clients (eg: with 2 privateslots: 2 -> 12 -> 0)
* many "A demo is already being recorded/played. Use demo_stop and retry." messages printed when playing a demo client-side.
* remove developer prints
* ExcessivePlus: when replaying a demo, democlients are not spectatable anymore after a variable amount of time, and are set to Away state. This is because of xp_inactivitySpectator timer. This was fixed by setting an appropriate localhost remote addr for the demo clients.
* when recording a demo and stopping it, the demo file is still left open and locked until the game/server is closed.
* port to the latest openarena engine based on the latest ioquake3 (should change the demoExt management in files.c).
* fix: big memory leaks, Z_Free pointer errors and removed a few useless mallocs. Thank's to Valgrind (use +set vm_game 1 to use Valgrind with OA, else with any other value it won't work).
* fix: svdemo filenames were truncated, now they should have more length to spare
* fix: Compatibility with OA 0.8.8: fix: fixed "FIXING ENT->S.NUMBER!!!" error, crashing demo playback with OA > 0.8.5. Now, the patch is compatible with OA 0.8.8
* fix: compatibility with maps containing mover objects (like moving platforms of Kaos2): "Reached_BinaryMover: bad moverState" error. Fixed by stopping ent->reached from being called by setting entity->s.pos.trType = TR_LINEAR when entity->s.pos.trType == TR_LINEAR_STOP.
* add: Compiled for ioquake3 latest version (but not yet merged in OA v3): https://github.com/lrq3000/ioq3/tree/server-side-demo
* In ExcessivePlus tournament, sometimes one of the two players won't be spectatable as first person as soon as the second player connects. The cause is unknown, but it happens when warmup is being recorded in the same demo (you can see a lot of InitGame and ShutdownGame). The new hooks might fix this? -> yes, the new hook fixed the issue by stopping the demo and starting a new one (in sv_autoDemo 1 mode), so that the warmup session is separated from the match in two different demos.
* After a demo of a warmup, all clients gets disconnected (with a hangout connection, they get an infinite "awaiting snapshot", so they have to do /reconnect) -> Fixed by the new hooks, now there are a few "awaiting snapshot" the time the server relaunches with old parameters (before demo playback) and the clients reconnect smoothly.
