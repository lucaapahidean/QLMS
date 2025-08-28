cmake_minimum_required(VERSION 3.19)
project(QLMSServer LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Network Sql)

qt_standard_project_setup()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

qt_add_executable(QLMSServer
    main.cpp
    user.h user.cpp
    coursematerial.h coursematerial.cpp
    question.h question.cpp
    databasemanager.h databasemanager.cpp
    clienthandler.h clienthandler.cpp
    server.h server.cpp
)

target_link_libraries(QLMSServer
    PRIVATE
        Qt::Core
        Qt::Network
        Qt::Sql
)

include(GNUInstallDirs)

install(TARGETS QLMSServer
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
