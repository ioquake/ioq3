project("basegame vm")

add_subdirectory("cgame")
add_subdirectory("game")
add_subdirectory("ui")

if(NOT ${CMAKE_VERSION} VERSION_LESS "3.3.0")

	set(BASE_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/basegame")
	set(PK3_FILENAME "basegame_vm.pk3")
	set(PK3_FILE "${BASE_DIR}/${PK3_FILENAME}")

	add_custom_target(dist_basegame_vm
		COMMAND ${CMAKE_COMMAND} -E chdir "${BASE_DIR}"
			tar "cfv" "${PK3_FILE}" --format=zip
			"vm/$<TARGET_FILE_NAME:${BASE_CGAME_VM_TARGET}>"
			"vm/$<TARGET_FILE_NAME:${BASE_GAME_VM_TARGET}>"
			"vm/$<TARGET_FILE_NAME:${BASE_UI_VM_TARGET}>"
		DEPENDS ${BASE_CGAME_VM_TARGET}
			${BASE_GAME_VM_TARGET}
			${BASE_UI_VM_TARGET}
		COMMENT ${PK3_FILE}
	)

endif()
