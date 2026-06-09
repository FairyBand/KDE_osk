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

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 DBus)
find_package(Qt6 6.4 QUIET COMPONENTS WaylandClient)
find_package(Qt6GuiPrivate 6.4 QUIET NO_MODULE)
find_package(Qt6WaylandClientPrivate 6.4 QUIET NO_MODULE)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUDEV REQUIRED IMPORTED_TARGET libudev)

option(KDE_OSK_BUILD_FCITX5_BRIDGE "Build the optional fcitx5 desktop focus bridge" ON)
option(KDE_OSK_BUILD_SDDM_INPUTMETHOD "Build the SDDM greeter Wayland input-method backend" ON)
option(KDE_OSK_USE_LAYER_SHELL "Use LayerShellQt for Wayland keyboard window placement" ON)
if(KDE_OSK_USE_LAYER_SHELL)
    find_package(LayerShellQt REQUIRED)
endif()
if(KDE_OSK_BUILD_FCITX5_BRIDGE)
    find_package(Fcitx5Core QUIET)
    find_package(Fcitx5Utils QUIET)
endif()
if(KDE_OSK_BUILD_SDDM_INPUTMETHOD)
    find_package(ECM QUIET NO_MODULE)
    if(ECM_FOUND)
        list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
        find_package(WaylandProtocols 1.19 QUIET)
    endif()
    if(TARGET Qt6::WaylandClientPrivate)
        get_target_property(KDE_OSK_QT_WAYLANDCLIENT_PRIVATE_INCLUDE_DIRS
            Qt6::WaylandClientPrivate
            INTERFACE_INCLUDE_DIRECTORIES)
        find_path(KDE_OSK_QT_WAYLANDCLIENT_PRIVATE_ROOT
            NAMES QtWaylandClient/private/qwaylandwindow_p.h
            HINTS ${KDE_OSK_QT_WAYLANDCLIENT_PRIVATE_INCLUDE_DIRS}
            NO_DEFAULT_PATH)
        if(NOT KDE_OSK_QT_WAYLANDCLIENT_PRIVATE_ROOT)
            find_path(KDE_OSK_QT_WAYLANDCLIENT_PRIVATE_ROOT
                NAMES QtWaylandClient/private/qwaylandwindow_p.h)
        endif()
    endif()
endif()

qt_standard_project_setup(REQUIRES 6.4)

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

configure_file(
    data/kde-osk-sddm-wayland.conf.in
    ${CMAKE_CURRENT_BINARY_DIR}/kde-osk-sddm-wayland.conf
    @ONLY
)

qt_add_executable(kde-osk-shell
    src/main.cpp
    src/KeyboardController.cpp
    src/KeyboardController.h
    src/HardwareKeyboardMonitor.cpp
    src/HardwareKeyboardMonitor.h
    src/UInputKeyboard.cpp
    src/UInputKeyboard.h
    src/ShellWindowAdapter.cpp
    src/ShellWindowAdapter.h
    src/ShellSettings.cpp
    src/ShellSettings.h
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

if(KDE_OSK_USE_LAYER_SHELL AND TARGET LayerShellQt::Interface)
    target_compile_definitions(kde-osk-shell PRIVATE HAVE_LAYER_SHELL_QT)
    target_link_libraries(kde-osk-shell PRIVATE LayerShellQt::Interface)
elseif(KDE_OSK_USE_LAYER_SHELL)
    message(FATAL_ERROR "LayerShellQt was found, but target LayerShellQt::Interface is missing.")
else()
    message(WARNING "LayerShellQt was not found. KDE OSK will build, but Wayland positioning and non-focus behavior will be limited.")
endif()

if(KDE_OSK_BUILD_FCITX5_BRIDGE AND TARGET Fcitx5::Core AND TARGET Fcitx5::Utils)
    add_library(fcitx5-kde-osk-bridge MODULE
        src/fcitx5-bridge/KdeOskBridge.cpp
    )
    target_link_libraries(fcitx5-kde-osk-bridge
        PRIVATE
            Fcitx5::Core
            Fcitx5::Utils
    )
    set_target_properties(fcitx5-kde-osk-bridge PROPERTIES
        PREFIX "lib"
    )
    install(TARGETS fcitx5-kde-osk-bridge
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/fcitx5
    )
    install(FILES data/fcitx5-kde-osk-bridge.conf
        DESTINATION ${CMAKE_INSTALL_DATADIR}/fcitx5/addon
    )
elseif(KDE_OSK_BUILD_FCITX5_BRIDGE)
    message(WARNING "Fcitx5 development files were not found. The fcitx5 bridge will not be built.")
endif()

if(KDE_OSK_BUILD_SDDM_INPUTMETHOD
    AND TARGET Qt6::WaylandClient
    AND TARGET Qt6::WaylandClientPrivate
    AND TARGET Qt6::GuiPrivate
    AND KDE_OSK_QT_WAYLANDCLIENT_PRIVATE_ROOT
    AND WaylandProtocols_FOUND
    AND COMMAND qt_generate_wayland_protocol_client_sources)
    qt_add_executable(kde-osk-sddm-inputmethod
        src/HardwareKeyboardMonitor.cpp
        src/HardwareKeyboardMonitor.h
        src/sddm/InputPanelIntegration.cpp
        src/sddm/InputPanelIntegration.h
        src/sddm/InputPanelRole.h
        src/sddm/QWaylandInputPanelShellIntegration.cpp
        src/sddm/QWaylandInputPanelShellIntegration.h
        src/sddm/QWaylandInputPanelSurface.cpp
        src/sddm/QWaylandInputPanelSurface.h
        src/sddm/SddmInputMethodController.cpp
        src/sddm/SddmInputMethodController.h
        src/sddm/SddmInputMethodMain.cpp
        src/sddm/WaylandInputMethod.cpp
        src/sddm/WaylandInputMethod.h
    )

    qt_add_qml_module(kde-osk-sddm-inputmethod
        URI KdeOsk.SddmInputMethod
        VERSION 1.0
        QML_FILES
            qml/SddmInputMethod.qml
            qml/KeyboardView.qml
            qml/KeyButton.qml
    )

    qt_generate_wayland_protocol_client_sources(kde-osk-sddm-inputmethod
        FILES
            ${WaylandProtocols_DATADIR}/unstable/input-method/input-method-unstable-v1.xml
    )

    target_link_libraries(kde-osk-sddm-inputmethod
        PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::GuiPrivate
            Qt6::Qml
            Qt6::Quick
            Qt6::QuickControls2
            Qt6::WaylandClient
            Qt6::WaylandClientPrivate
            PkgConfig::LIBUDEV
    )

    target_include_directories(kde-osk-sddm-inputmethod PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

    install(TARGETS kde-osk-sddm-inputmethod
        BUNDLE DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
elseif(KDE_OSK_BUILD_SDDM_INPUTMETHOD)
    message(WARNING "Qt Wayland client private headers, wayland-protocols, or Qt Wayland CMake code generation support were not found. The SDDM input-method backend will not be built.")
endif()

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

install(FILES data/70-kde-osk-uinput.rules
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/udev/rules.d
)

if(TARGET kde-osk-sddm-inputmethod)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kde-osk-sddm-wayland.conf
        DESTINATION ${CMAKE_INSTALL_DATADIR}/kde-osk/sddm
    )

    install(PROGRAMS scripts/install-sddm-inputmethod.sh
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

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
