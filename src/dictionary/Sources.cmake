set( krossword_SRCS ${krossword_SRCS}
    dictionary/dictionarymanager.cpp
    dictionary/dictionarymodel.cpp
    dictionary/dictionarygui.cpp
)

kde4_add_ui_files( krossword_SRCS
    dictionary/dictionarygui.ui
    dictionary/database_connection.ui )