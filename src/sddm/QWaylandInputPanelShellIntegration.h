#pragma once

#include <QtWaylandClient/private/qwaylandshellintegration_p.h>

#include <qwayland-input-method-unstable-v1.h>

class QWaylandInputPanelShellIntegration : public QtWaylandClient::QWaylandShellIntegrationTemplate<QWaylandInputPanelShellIntegration>,
                                           public QtWayland::zwp_input_panel_v1
{
public:
    QWaylandInputPanelShellIntegration();
    ~QWaylandInputPanelShellIntegration() override;

    QtWaylandClient::QWaylandShellSurface *createShellSurface(QtWaylandClient::QWaylandWindow *window) override;
};
