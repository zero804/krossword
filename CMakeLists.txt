project(krossword)
cmake_minimum_required(VERSION 3.8)

set(QT_MIN_VERSION "5.2.0")

find_package(ECM 1.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED Core Widgets PrintSupport Sql)
find_package(KF5 REQUIRED COMPONENTS Archive Config CoreAddons KIO XmlGui I18n Completion IconThemes KDELibs4Support)

find_package(KF5KDEGames 4.9.0 REQUIRED)

set(SHARED_MIME_INFO_MINIMUM_VERSION "0.30")
find_package(SharedMimeInfo)

include(FeatureSummary)
include(WriteBasicConfigVersionFile)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(ECMInstallIcons)

add_definitions(-DQT_NO_URL_CAST_FROM_STRING) #for kurl porting

add_subdirectory(src)
add_subdirectory(icons)
add_subdirectory(themes)
add_subdirectory(templates)
add_subdirectory(crosswordthumbcreator)

add_subdirectory(po)

########### install mimetype file ###############
install(FILES krossword.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})
