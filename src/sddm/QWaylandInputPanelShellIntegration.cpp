#include "QWaylandInputPanelShellIntegration.h"

#include "QWaylandInputPanelSurface.h"

#include <QtWaylandClient/private/qwaylandwindow_p.h>

QWaylandInputPanelShellIntegration::QWaylandInputPanelShellIntegration()
    : QtWaylandClient::QWaylandShellIntegrationTemplate<QWaylandInputPanelShellIntegration>(1)
{
}

QWaylandInputPanelShellIntegration::~QWaylandInputPanelShellIntegration() = default;

QtWaylandClient::QWaylandShellSurface *QWaylandInputPanelShellIntegration::createShellSurface(QtWaylandClient::QWaylandWindow *window)
{
    if (!isActive()) {
        return nullptr;
    }

    auto *surface = get_input_panel_surface(window->wlSurface());
    return new QWaylandInputPanelSurface(surface, window);
}
