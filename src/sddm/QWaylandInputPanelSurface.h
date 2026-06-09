#pragma once

#include <QtWaylandClient/private/qwaylandshellsurface_p.h>

#include <qwayland-input-method-unstable-v1.h>

class QWaylandInputPanelSurface : public QtWaylandClient::QWaylandShellSurface, public QtWayland::zwp_input_panel_surface_v1
{
public:
    QWaylandInputPanelSurface(struct ::zwp_input_panel_surface_v1 *surface, QtWaylandClient::QWaylandWindow *window);
    ~QWaylandInputPanelSurface() override;

    void applyConfigure() override;
};
