cmake_minimum_required(VERSION 3.19)
project(QLMSServer LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Network Sql)

qt_standard_project_setup()

qt_add_executable(QLMSServer
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
