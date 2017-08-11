project( crosswordthumbcreator )

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

add_library(crosswordthumbnail MODULE ${crosswordthumbnail_SRCS} )
target_link_libraries( crosswordthumbnail KF5::KIOCore )

install( TARGETS crosswordthumbnail DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES crosswordthumbnail.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
