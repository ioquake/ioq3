project("missionpack vm")

add_subdirectory("cgame")
add_subdirectory("game")
add_subdirectory("ui")

if(NOT ${CMAKE_VERSION} VERSION_LESS "3.3.0")

	set(MISSION_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/missionpack")
	set(PK3_FILENAME "missionpack_vm.pk3")
	set(PK3_FILE "${MISSION_DIR}/${PK3_FILENAME}")

	add_custom_target(dist_missionpack_vm
		COMMAND ${CMAKE_COMMAND} -E chdir "${MISSION_DIR}"
			tar "cfv" "${PK3_FILE}" --format=zip
			"vm/$<TARGET_FILE_NAME:${MISSION_CGAME_VM_TARGET}>"
			"vm/$<TARGET_FILE_NAME:${MISSION_GAME_VM_TARGET}>"
			"vm/$<TARGET_FILE_NAME:${MISSION_UI_VM_TARGET}>"
		DEPENDS ${MISSION_CGAME_VM_TARGET}
			${MISSION_GAME_VM_TARGET}
			${MISSION_UI_VM_TARGET}
		COMMENT ${PK3_FILE}
	)

endif()
