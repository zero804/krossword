include_directories(${CMAKE_CURRENT_SOURCE_DIR}) # kgrid2d.h

set(krossword_SRCS
   main.cpp
   mainwindow.cpp
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
   headeritem.cpp
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
    move_cells.ui
    dialogs/printpreviewdialog.ui)

kconfig_add_kcfg_files(krossword_SRCS settings.kcfgc)

qt5_add_resources(krossword_SRCS krossword_ui.qrc)

add_executable(krossword ${krossword_SRCS})

target_link_libraries(krossword
    Qt5::Widgets Qt5::Sql Qt5::PrintSupport
    KF5::Archive KF5::XmlGui KF5::I18n KF5::Completion KF5::KIOCore
    KF5::KIOWidgets KF5::KIOFileWidgets KF5::JobWidgets KF5::KIONTLM
    KF5::IconThemes KF5::TextWidgets
    KF5KDEGames)

install(TARGETS krossword ${INSTALL_TARGETS_DEFAULT_ARGS})


########### install files ###############

install(FILES krossword.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES krossword.kcfg  DESTINATION  ${KCFG_INSTALL_DIR})
