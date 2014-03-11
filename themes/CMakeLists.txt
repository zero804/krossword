
set( krosswordpuzzle_THEME_SRCS
	default.desktop
	default.svgz
	default.png

	scribble.desktop
	scribble.svgz
	scribble.png )

install( FILES ${krosswordpuzzle_THEME_SRCS}
	 DESTINATION ${DATA_INSTALL_DIR}/krosswordpuzzle/themes )
