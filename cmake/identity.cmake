set(PROJECT_NAME ioq3 CACHE STRING "Project name")
set(PROJECT_VERSION 1.36 CACHE STRING "Project version")

set(SERVER_NAME ioq3ded CACHE STRING "Base name for dedicated server executable")
set(CLIENT_NAME ioquake3 CACHE STRING "Base name for client executable")

set(BASEGAME baseq3 CACHE STRING "Default game data folder name")

set(CGAME_MODULE cgame CACHE STRING "Base name for CGame module")
set(GAME_MODULE qagame CACHE STRING "Base name for Game module")
set(UI_MODULE ui CACHE STRING "Base name for UI module")

set(WINDOWS_ICON_PATH ${CMAKE_SOURCE_DIR}/misc/windows/quake3.ico CACHE STRING "Path to icon file (ICO) for Windows executable")

set(MACOS_ICON_PATH ${CMAKE_SOURCE_DIR}/misc/macos/quake3_flat.icns CACHE STRING "Path to icon file (ICNS) for Mac executable")
set(MACOS_BUNDLE_ID org.ioquake.${CLIENT_NAME} CACHE STRING "Mac bundle ID")

set(COPYRIGHT "QUAKE III ARENA Copyright © 1999-2000 id Software, Inc. All rights reserved." CACHE STRING "Coypright notice")

set(CONTACT_EMAIL "info@ioquake.org" CACHE STRING "Contact email address")
set(PROTOCOL_HANDLER quake3 CACHE STRING "Protocol handler name")

set(PRODUCT_NAME ${PROJECT_NAME} CACHE STRING "Product name")
set(CLIENT_WINDOW_TITLE ${CLIENT_NAME} CACHE STRING "Client window title")
set(CLIENT_WINDOW_MIN_TITLE ${PRODUCT_NAME} CACHE STRING "Client window short title")
set(HOMEPATH_NAME_UNIX_LEGACY ".q3a" CACHE STRING "UNIX legacy configuration folder name (will be placed in $HOME)")
set(HOMEPATH_NAME "Quake3" CACHE STRING "Configuration folder name")
set(GAMENAME_FOR_MASTER "Quake3Arena" CACHE STRING "Game name for master server (must not contain spaces!)")
set(CINEMATICS_LOGO "idlogo.RoQ" CACHE STRING "Logo video filename (set to blank string to disable)")
set(CINEMATICS_INTRO "intro.RoQ" CACHE STRING "Intro video filename (set to blank string to disable)")
option(LEGACY_PROTOCOL "Use legacy network protocol" ON)
set(CONFIG_PREFIX "q3config" CACHE STRING "Base name for config file")
set(DEMOEXT "dm_" CACHE STRING "Demo filename extension")
