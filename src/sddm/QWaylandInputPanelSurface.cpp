#include "QWaylandInputPanelSurface.h"

#include <QDebug>
#include <QtWaylandClient/private/qwaylandscreen_p.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>

QWaylandInputPanelSurface::QWaylandInputPanelSurface(struct ::zwp_input_panel_surface_v1 *surface, QtWaylandClient::QWaylandWindow *window)
    : QtWaylandClient::QWaylandShellSurface(window)
    , QtWayland::zwp_input_panel_surface_v1(surface)
{
    window->applyConfigureWhenPossible();
}

QWaylandInputPanelSurface::~QWaylandInputPanelSurface() = default;

void QWaylandInputPanelSurface::applyConfigure()
{
    auto *screen = window()->waylandScreen();
    if (screen == nullptr) {
        qWarning() << "Cannot place KDE OSK input panel without a Wayland screen.";
        return;
    }

    set_toplevel(screen->output(), position_center_bottom);
    window()->display()->handleWindowActivated(window());
}
