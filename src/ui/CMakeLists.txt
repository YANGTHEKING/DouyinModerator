set(UI_SOURCES
    EventLogPanel.cpp
    RuleEditorPanel.cpp
    ConfigPanel.cpp
    LoginDialog.cpp
    MainWindow.cpp
)

set(UI_HEADERS
    EventLogPanel.h
    RuleEditorPanel.h
    ConfigPanel.h
    LoginDialog.h
    MainWindow.h
)

add_library(ui_lib STATIC ${UI_SOURCES} ${UI_HEADERS})
target_include_directories(ui_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ui_lib PUBLIC core_lib)
