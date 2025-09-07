cmake_minimum_required(VERSION 3.19)
project(QLMSClient LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Network Widgets)

qt_standard_project_setup()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

qt_add_executable(QLMSClient
    main.cpp
    networkmanager.h networkmanager.cpp
    logindialog.h logindialog.cpp
    basedashboardwindow.h basedashboardwindow.cpp
    usermanagementwidget.h usermanagementwidget.cpp
    admindashboard.h admindashboard.cpp
    gradingwidget.h gradingwidget.cpp
    instructordashboard.h instructordashboard.cpp
    courselistwidget.h courselistwidget.cpp
    studentdashboard.h studentdashboard.cpp
    coursemanagementwidget.h coursemanagementwidget.cpp
    materialcreationwizard.h materialcreationwizard.cpp
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
