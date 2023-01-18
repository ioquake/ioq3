project("basegame qvm")

add_subdirectory("cgame")
add_subdirectory("game")
add_subdirectory("ui")

if(NOT ${CMAKE_VERSION} VERSION_LESS "3.3.0")

	get_target_property(CGAME_FILENAME ${BASE_CGAME_QVM_TARGET} QVM_FILENAME)
	get_target_property(GAME_FILENAME ${BASE_GAME_QVM_TARGET} QVM_FILENAME)
	get_target_property(UI_FILENAME ${BASE_UI_QVM_TARGET} QVM_FILENAME)

	set(BASE_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/basegame")
	set(PK3_FILENAME "basegame_qvm.pk3")
	set(PK3_FILE "${BASE_DIR}/${PK3_FILENAME}")

	add_custom_target(dist_basegame_qvm
		COMMAND ${CMAKE_COMMAND} -E chdir "${BASE_DIR}"
			tar "cfv" "${PK3_FILE}" --format=zip
			"vm/${CGAME_FILENAME}"
			"vm/${GAME_FILENAME}"
			"vm/${UI_FILENAME}"
		DEPENDS	"${BASE_DIR}/vm/${CGAME_FILENAME}"
			"${BASE_DIR}/vm/${GAME_FILENAME}"
			"${BASE_DIR}/vm/${UI_FILENAME}"
		COMMENT ${PK3_FILE}
	)

endif()
