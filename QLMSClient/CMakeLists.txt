cmake_minimum_required(VERSION 3.19)
project(QLMSClient LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Network Widgets)

qt_standard_project_setup()

qt_add_executable(QLMSClient
)

target_link_libraries(QLMSClient
    PRIVATE
        Qt::Core
        Qt::Network
        Qt::Widgets
)

include(GNUInstallDirs)

install(TARGETS QLMSClient
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
