include_directories(${CMAKE_CURRENT_SOURCE_DIR}) # kgrid2d.h

set(krossword_SRCS
   krosswordpuzzle.cpp
   main.cpp
   global.cpp
   crosswordxmlguiwindow.cpp
   animator.cpp
   krosswordtheme.cpp
   commands.cpp
   krosswordpuzzleview.cpp
   krosswordpuzzlescene.cpp
   krossword.cpp
   krosswordrenderer.cpp
   krossworddocument.cpp
   cluemodel.cpp
   htmldelegate.cpp
   dictionary.cpp
   extendedsqltablemodel.cpp
   clueexpanderitem.cpp
   templatemodel.cpp
)

INCLUDE (io/Sources.cmake)
INCLUDE (cells/Sources.cmake)
INCLUDE (dialogs/Sources.cmake)
INCLUDE (dialogs/cellwidgets/Sources.cmake)
INCLUDE (library/Sources.cmake)

ki18n_wrap_ui(krossword_SRCS
    settings.ui
    create_new.ui
    properties.ui
    clue_properties_dock.ui
    image_properties_dock.ui
    solution_letter_properties.ui
    print_crossword.ui
    export_to_image.ui
    dictionaries.ui
    database_connection.ui
    convert_crossword_type.ui
    clue_number_mapping.ui
    download.ui
    configure_details.ui
    move_cells.ui)

kconfig_add_kcfg_files(krossword_SRCS settings.kcfgc)

add_executable(krossword ${krossword_SRCS})

target_link_libraries(krossword
    Qt5::Widgets Qt5::Sql Qt5::PrintSupport
    KF5::XmlGui KF5::I18n KF5::Completion KF5::KIOCore KF5::KIOWidgets KF5::KIOFileWidgets KF5::KIONTLM KF5::IconThemes KF5::KDELibs4Support
    KF5KDEGames)

install(TARGETS krossword ${INSTALL_TARGETS_DEFAULT_ARGS})


########### install files ###############

install(FILES krossword.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES krossword.kcfg  DESTINATION  ${KCFG_INSTALL_DIR})
install(FILES krosswordui.rc
               krossword_library_ui.rc
               krossword_crossword_ui.rc
         DESTINATION ${DATA_INSTALL_DIR}/krossword)
install(FILES krossword.knsrc DESTINATION ${CONFIG_INSTALL_DIR})
