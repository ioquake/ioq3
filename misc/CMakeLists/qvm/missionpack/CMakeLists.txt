project("missionpack qvm")

add_subdirectory("cgame")
add_subdirectory("game")
add_subdirectory("ui")

if(NOT ${CMAKE_VERSION} VERSION_LESS "3.3.0")

	get_target_property(CGAME_FILENAME ${MISSION_CGAME_QVM_TARGET} QVM_FILENAME)
	get_target_property(GAME_FILENAME ${MISSION_GAME_QVM_TARGET} QVM_FILENAME)
	get_target_property(UI_FILENAME ${MISSION_UI_QVM_TARGET} QVM_FILENAME)

	set(MISSION_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/missionpack")
	set(PK3_FILENAME "missionpack_qvm.pk3")
	set(PK3_FILE "${MISSION_DIR}/${PK3_FILENAME}")

	add_custom_target(dist_missionpack_qvm
		COMMAND ${CMAKE_COMMAND} -E chdir "${MISSION_DIR}"
			tar "cfv" "${PK3_FILE}" --format=zip 
			"vm/${CGAME_FILENAME}"
			"vm/${GAME_FILENAME}"
			"vm/${UI_FILENAME}"
		DEPENDS	"${MISSION_DIR}/vm/${CGAME_FILENAME}"
			"${MISSION_DIR}/vm/${GAME_FILENAME}"
			"${MISSION_DIR}/vm/${UI_FILENAME}"
		COMMENT ${PK3_FILE}
	)

endif()