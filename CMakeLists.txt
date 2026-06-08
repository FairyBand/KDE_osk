cmake_minimum_required(VERSION 3.24)

project(KdeOsk
    VERSION 0.1.0
    DESCRIPTION "Wayland-first KDE Plasma on-screen keyboard"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

include(GNUInstallDirs)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 DBus)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUDEV REQUIRED IMPORTED_TARGET libudev)

qt_standard_project_setup(REQUIRES 6.5)

configure_file(
    data/org.kde.KdeOsk.service.in
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.KdeOsk.service
    @ONLY
)

configure_file(
    data/kde-osk-shell.service.in
    ${CMAKE_CURRENT_BINARY_DIR}/kde-osk-shell.service
    @ONLY
)

qt_add_executable(kde-osk-shell
    src/main.cpp
    src/KeyboardController.cpp
    src/KeyboardController.h
    src/HardwareKeyboardMonitor.cpp
    src/HardwareKeyboardMonitor.h
)

qt_add_qml_module(kde-osk-shell
    URI KdeOsk
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/KeyboardView.qml
        qml/KeyButton.qml
)

target_link_libraries(kde-osk-shell
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::DBus
        PkgConfig::LIBUDEV
)

install(TARGETS kde-osk-shell
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.KdeOsk.service
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/services
)

install(FILES data/kde-osk-shell.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kde-osk-shell.service
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/systemd/user
)

install(FILES data/org.kde.KdeOsk.Keyboard.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/interfaces
)

install(DIRECTORY qml/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/kde-osk/qml
    FILES_MATCHING PATTERN "*.qml"
)

install(FILES
    docs/ARCHITECTURE.md
    docs/DBUS.md
    docs/IMPLEMENTATION_PLAN.md
    docs/LINUX_TEST_PLAN.md
    docs/PRODUCT_SPEC.zh-CN.md
    docs/REFERENCES.md
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)
