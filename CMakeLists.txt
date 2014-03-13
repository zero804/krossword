project(krosswordpuzzle)

find_package(KDE4 REQUIRED)
include(KDE4Defaults)

find_package(KDEGames REQUIRED)

# for KDE 4.3 we require at least version 0.30
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
set(SHARED_MIME_INFO_MINIMUM_VERSION "0.30")
find_package(SharedMimeInfo)

include_directories(${KDE4_INCLUDES} ${QT_INCLUDES})

add_subdirectory(src)
add_subdirectory(icons)
add_subdirectory(themes)
add_subdirectory(templates)
add_subdirectory(crosswordthumbcreator)

# This installs translations for some selected languages from KDE SVN
# You can comment out translations you do not need in "l10n/CMakeLists.txt"
# add_subdirectory( l10n )

########### install mimetype file ###############
install(FILES krosswordpuzzle.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})
