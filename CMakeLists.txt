project(krossword)

find_package(KDE4 REQUIRED)
include(KDE4Defaults)

find_package(KDEGames REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
set(SHARED_MIME_INFO_MINIMUM_VERSION "0.30")
find_package(SharedMimeInfo)

include_directories(${KDE4_INCLUDES} ${QT_INCLUDES})

add_subdirectory(src)
add_subdirectory(icons)
add_subdirectory(themes)
add_subdirectory(templates)
add_subdirectory(crosswordthumbcreator)

add_subdirectory(po)

########### install mimetype file ###############
install(FILES krossword.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})
