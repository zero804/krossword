set( krossword_THEME_SRCS
    default.desktop
    default.svgz
    default.png

    scribble.desktop
    scribble.svgz
    scribble.png

    egyptian.desktop
    egyptian.svgz
    egyptian.png 

    ink.desktop
    ink.svgz
    ink.png)

install( FILES ${krossword_THEME_SRCS}
    DESTINATION ${DATA_INSTALL_DIR}/krossword/themes )
