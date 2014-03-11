project( crosswordthumbcreator )

find_package( KDE4 REQUIRED )
include( KDE4Defaults )
include( MacroOptionalAddSubdirectory )
include_directories( ${KDE4_INCLUDES} ${QT_INCLUDES} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../src) # kgrid2d.h

set( crosswordthumbnail_SRCS crosswordthumbnail.cpp
    krossword.cpp
    krosswordpuzreader.cpp
    krosswordxmlreader.cpp
    cells/krosswordcell.cpp
    cells/cluecell.cpp
    cells/lettercell.cpp
    cells/spannedcell.cpp
    cells/imagecell.cpp )

# kde4_add_ui_files( filetypethumbnail_SRCS config.ui )

kde4_add_plugin( crosswordthumbnail ${crosswordthumbnail_SRCS} )
target_link_libraries( crosswordthumbnail ${KDE4_KIO_LIBS} )

install( TARGETS crosswordthumbnail DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES crosswordthumbnail.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
