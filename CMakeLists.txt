cmake_minimum_required(VERSION 3.24)

project(KdeOsk
    VERSION 0.1.0
    DESCRIPTION "Wayland-first KDE Plasma on-screen keyboard"
    LANGUAGES C CXX
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
option(KDE_OSK_BUILD_KWIN_BROKER "Build the KDE Plasma virtual-keyboard broker" ON)
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
        find_package(WaylandScanner QUIET)
    endif()
    find_package(Qt6WaylandScannerTools 6.4 QUIET CONFIG)
    if(NOT TARGET Qt6::qtwaylandscanner)
        find_program(KDE_OSK_QTWAYLANDSCANNER_EXECUTABLE
            NAMES qtwaylandscanner
            HINTS
                /usr/lib/qt6
                /usr/lib/qt6/bin
                /usr/lib/qt6/libexec
                /usr/bin)
        if(KDE_OSK_QTWAYLANDSCANNER_EXECUTABLE)
            add_executable(Qt6::qtwaylandscanner IMPORTED)
            set_target_properties(Qt6::qtwaylandscanner PROPERTIES
                IMPORTED_LOCATION "${KDE_OSK_QTWAYLANDSCANNER_EXECUTABLE}")
        endif()
    endif()
    if(Qt6WaylandClient_DIR AND EXISTS "${Qt6WaylandClient_DIR}/Qt6WaylandClientMacros.cmake")
        include("${Qt6WaylandClient_DIR}/Qt6WaylandClientMacros.cmake")
    endif()
    set(KDE_OSK_WAYLAND_PROTOCOL_HINTS
        ${WaylandProtocols_DATADIR}
        ${WaylandProtocols_DATA_DIR}
        /usr/share/wayland-protocols
        /usr/local/share/wayland-protocols)
    find_path(KDE_OSK_WAYLAND_PROTOCOLS_ROOT
        NAMES unstable/input-method/input-method-unstable-v1.xml
        HINTS ${KDE_OSK_WAYLAND_PROTOCOL_HINTS})
    if(KDE_OSK_WAYLAND_PROTOCOLS_ROOT)
        set(KDE_OSK_INPUT_METHOD_PROTOCOL
            "${KDE_OSK_WAYLAND_PROTOCOLS_ROOT}/unstable/input-method/input-method-unstable-v1.xml")
    endif()
    if(TARGET Qt6::WaylandClientPrivate)
        get_target_property(KDE_OSK_QT_WAYLANDCLIENT_PRIVATE_INCLUDE_DIRS
            Qt6::WaylandClientPrivate
            INTERFACE_INCLUDE_DIRECTORIES)
    endif()
    set(KDE_OSK_QT_WAYLANDCLIENT_PRIVATE_HINTS
        ${Qt6WaylandClient_PRIVATE_INCLUDE_DIRS}
        ${Qt6WaylandClient_INCLUDE_DIRS}
        ${KDE_OSK_QT_WAYLANDCLIENT_PRIVATE_INCLUDE_DIRS}
        /usr/include
        /usr/local/include)
    set(KDE_OSK_QT_WAYLANDCLIENT_PRIVATE_CLEAN_HINTS)
    foreach(includeDir IN LISTS KDE_OSK_QT_WAYLANDCLIENT_PRIVATE_HINTS)
        if(includeDir AND NOT includeDir MATCHES "^\\$<")
            list(APPEND KDE_OSK_QT_WAYLANDCLIENT_PRIVATE_CLEAN_HINTS "${includeDir}")
        endif()
    endforeach()
    find_path(KDE_OSK_QT_WAYLANDCLIENT_PRIVATE_ROOT
        NAMES QtWaylandClient/private/qwaylandwindow_p.h
        HINTS ${KDE_OSK_QT_WAYLANDCLIENT_PRIVATE_CLEAN_HINTS}
        PATH_SUFFIXES
            QtWaylandClient/${Qt6_VERSION}
            QtWaylandClient/${Qt6WaylandClient_VERSION}
            qt6/QtWaylandClient/${Qt6_VERSION}
            qt6/QtWaylandClient/${Qt6WaylandClient_VERSION})
endif()

set(KDE_OSK_SDDM_INPUTMETHOD_MISSING_DEPS)
if(KDE_OSK_BUILD_SDDM_INPUTMETHOD)
    if(NOT TARGET Qt6::WaylandClient)
        list(APPEND KDE_OSK_SDDM_INPUTMETHOD_MISSING_DEPS "Qt6::WaylandClient")
    endif()
    if(NOT TARGET Qt6::WaylandClientPrivate)
        list(APPEND KDE_OSK_SDDM_INPUTMETHOD_MISSING_DEPS "Qt6::WaylandClientPrivate")
    endif()
    if(NOT TARGET Qt6::GuiPrivate)
        list(APPEND KDE_OSK_SDDM_INPUTMETHOD_MISSING_DEPS "Qt6::GuiPrivate")
    endif()
    if(NOT KDE_OSK_QT_WAYLANDCLIENT_PRIVATE_ROOT)
        list(APPEND KDE_OSK_SDDM_INPUTMETHOD_MISSING_DEPS "QtWaylandClient private headers")
    endif()
    if(NOT KDE_OSK_INPUT_METHOD_PROTOCOL)
        list(APPEND KDE_OSK_SDDM_INPUTMETHOD_MISSING_DEPS "wayland-protocols")
    endif()
    if(NOT WaylandScanner_FOUND)
        list(APPEND KDE_OSK_SDDM_INPUTMETHOD_MISSING_DEPS "wayland-scanner")
    endif()
    if(NOT TARGET Qt6::qtwaylandscanner)
        list(APPEND KDE_OSK_SDDM_INPUTMETHOD_MISSING_DEPS "qtwaylandscanner")
    endif()
    if(NOT COMMAND qt6_generate_wayland_protocol_client_sources)
        list(APPEND KDE_OSK_SDDM_INPUTMETHOD_MISSING_DEPS "Qt Wayland client protocol CMake macros")
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
    data/kde-osk-kwin-broker.desktop.in
    ${CMAKE_CURRENT_BINARY_DIR}/kde-osk-kwin-broker.desktop
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

if(KDE_OSK_BUILD_KWIN_BROKER)
    qt_add_executable(kde-osk-kwin-broker
        src/broker/KWinBrokerMain.cpp
    )
    target_link_libraries(kde-osk-kwin-broker
        PRIVATE
            Qt6::Core
            Qt6::DBus
    )
    install(TARGETS kde-osk-kwin-broker
        BUNDLE DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kde-osk-kwin-broker.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )
endif()

if(KDE_OSK_BUILD_SDDM_INPUTMETHOD
    AND NOT KDE_OSK_SDDM_INPUTMETHOD_MISSING_DEPS)
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

    qt6_generate_wayland_protocol_client_sources(kde-osk-sddm-inputmethod
        NO_INCLUDE_CORE_ONLY
        FILES
            ${KDE_OSK_INPUT_METHOD_PROTOCOL}
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

    target_include_directories(kde-osk-sddm-inputmethod
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_BINARY_DIR}
            ${KDE_OSK_QT_WAYLANDCLIENT_PRIVATE_ROOT}
    )

    install(TARGETS kde-osk-sddm-inputmethod
        BUNDLE DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    set(KDE_OSK_SDDM_INPUTMETHOD_BUILT TRUE)
elseif(KDE_OSK_BUILD_SDDM_INPUTMETHOD)
    string(REPLACE ";" ", " KDE_OSK_SDDM_INPUTMETHOD_MISSING_DEPS_TEXT "${KDE_OSK_SDDM_INPUTMETHOD_MISSING_DEPS}")
    message(WARNING "The SDDM input-method backend will not be built. Missing: ${KDE_OSK_SDDM_INPUTMETHOD_MISSING_DEPS_TEXT}")
    add_custom_target(kde-osk-sddm-inputmethod
        COMMAND ${CMAKE_COMMAND} -E echo "Cannot build kde-osk-sddm-inputmethod. Missing: ${KDE_OSK_SDDM_INPUTMETHOD_MISSING_DEPS_TEXT}"
        COMMAND ${CMAKE_COMMAND} -E false
    )
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

if(KDE_OSK_SDDM_INPUTMETHOD_BUILT)
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
    docs/BROKER.md
    docs/DBUS.md
    docs/IMPLEMENTATION_PLAN.md
    docs/LINUX_TEST_PLAN.md
    docs/PRODUCT_SPEC.zh-CN.md
    docs/REFERENCES.md
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)
