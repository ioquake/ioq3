//
//  net_humblenet.c
//  ioquake
//
//  Created by Chris Rudd on 3/9/16.
//

//#include <humblenet_p2p.h>

char* NET_ErrorString();

static PeerId humblenet_peer = 0;
static cvar_t *net_peer_server;
static cvar_t *net_server_name;
static cvar_t *net_peer_relay;
static qboolean published = 0;

static qboolean is_server_active() {
    return com_sv_running->value;
}

static void NET_peer_server_changed(struct cvar_s *self, char *oldValue, char *newValue) {
	if( ! humblenet_p2p_is_initialized() && newValue && *newValue ) {
		Com_Printf("initializing p2p network...\n");
		if( ! humblenet_p2p_init( newValue, "ioquake", "ioquake-secret", NULL ) ) {
			Com_Printf("Error connecting to signal server: \"%s\"\n", NET_ErrorString());
		}
	} else if( humblenet_p2p_is_initialized() ) {
		Com_Printf("New peer server will take affect after next restart");
	}
}

static void NET_server_name_changed(struct cvar_s *self, char *oldValue, char *newValue) {
    if( !humblenet_p2p_is_initialized() )
        return;

    if( published )
        humblenet_p2p_unregister_alias(NULL);

	if( newValue && *newValue && is_server_active() ) {
		humblenet_p2p_register_alias( newValue );
        
        published = qtrue;
		
		Com_Printf("Public server name changed to \"%s\"\n", newValue);
	}

    self->modified = qfalse;
}

//=============================================================================

#ifdef EMSCRIPTEN
#include <emscripten/html5.h>
const char* emUnloadCallback(int eventType, const void* reserved, void* userData) {
	Com_Quit_f();
	return "";
}
#endif

/*
 ====================
 HUMBLE_Init
 ====================
 */
void HUMBLENET_Init (void)
{
	if( humblenet_p2p_is_initialized() )
		return;
	
#ifdef EMSCRIPTEN
	
	// on emscripten this is supposed to be specified by the server
	// don't save to config
	int net_peer_server_flags = CVAR_ARCHIVE;
	
#else  // EMSCRIPTEN
	
	// on native user must set it
	// save to config
	int net_peer_server_flags = CVAR_ARCHIVE;
	
#endif  // EMSCRIPRTEN

	net_peer_server = Cvar_Get("net_peer_server", "ws://localhost:8080/ws", net_peer_server_flags);
//	net_peer_server->changed = NET_peer_server_changed;
	
	// fake a change so we initialize
	NET_peer_server_changed( net_peer_server, NULL, net_peer_server->string );
	
	net_server_name = Cvar_Get("net_server_name", "", CVAR_ARCHIVE);
//	net_server_name->changed = NET_server_name_changed;
	
	if( !humblenet_p2p_is_initialized() ) {
		Com_Printf("P2P Network not initialized. <insert instructions to do so here>...\n");
	}

    net_peer_relay = Cvar_Get("net_peer_relay", "0", CVAR_ARCHIVE);
    if( net_peer_relay->integer )
        humblenet_set_hint("p2p_use_relay", net_peer_relay->string);
	
#ifdef EMSCRIPTEN
	// disconnect client when user closes tab
	emscripten_set_beforeunload_callback(NULL, emUnloadCallback);
#endif
}

void HUMBLENET_Update(void) {
    if( ! humblenet_p2p_is_initialized() )
        return;

	humblenet_p2p_wait(0);
	
	if( humblenet_peer == 0 ) {
		humblenet_peer = humblenet_p2p_get_my_peer_id();
		if( humblenet_peer != 0) {
			Com_Printf("Assigned PeerId: %u...\n", humblenet_peer);
		}
    }
    else if( is_server_active() ) {
        if( net_server_name->modified || ( !published && *net_server_name->string ) )
            NET_server_name_changed(net_server_name, NULL, net_server_name->string );
    } else if( published ) {
        NET_server_name_changed(net_server_name, NULL, net_server_name->string );
        published = qfalse;
    }

    if( net_peer_relay->modified ) {
        humblenet_set_hint("p2p_use_relay", net_peer_relay->string);
        net_peer_relay->modified = qfalse;
    }
}

#ifndef HAVE_NET_Update
#define HAVE_NET_Update 
void NET_Update(void) {
    HUMBLENET_Update();
}
#endif

