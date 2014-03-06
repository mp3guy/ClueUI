cmake_minimum_required(VERSION 2.8)
project(ClueUI)
find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})

QT4_WRAP_UI(UI_CLUEUI_H 
            clueui.ui)

qt4_wrap_cpp(
        clueui_moc_SRCS
        clueui.h
        )

set(CMAKE_CXX_FLAGS "-g")

add_executable(ClueUI
               ${UI_CLUEUI_H}
               ${clueui_moc_SRCS}
               BoardModel.cpp
               clueui.cpp
               GameModel.cpp
               main.cpp
               )

TARGET_LINK_LIBRARIES(ClueUI 
                      ${QT_LIBRARIES})
