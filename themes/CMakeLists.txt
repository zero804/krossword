
set( krosswordpuzzle_THEME_SRCS
        classic.desktop
        classic.svgz
        classic.png

	default.desktop
	default.svgz
	default.png

	scribble.desktop
	scribble.svgz
	scribble.png

	egyptian.desktop
	egyptian.svgz
	egyptian.png )

install( FILES ${krosswordpuzzle_THEME_SRCS}
	 DESTINATION ${DATA_INSTALL_DIR}/krosswordpuzzle/themes )
