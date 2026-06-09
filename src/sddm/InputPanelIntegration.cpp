#include "InputPanelIntegration.h"

#include "QWaylandInputPanelShellIntegration.h"

#include <QDebug>
#include <QWindow>
#include <QtWaylandClient/private/qwaylandwindow_p.h>

bool initializeInputPanelSurface(QWindow *window, InputPanelRole::Role role)
{
    if (window == nullptr) {
        return false;
    }

    window->setProperty("kdeOskInputPanelRole", static_cast<int>(role));
    window->create();

    auto *waylandWindow = dynamic_cast<QtWaylandClient::QWaylandWindow *>(window->handle());
    if (waylandWindow == nullptr) {
        qWarning() << "SDDM input method window is not a Wayland window.";
        return false;
    }

    static QWaylandInputPanelShellIntegration *shellIntegration = nullptr;
    if (shellIntegration == nullptr) {
        shellIntegration = new QWaylandInputPanelShellIntegration;
        if (!shellIntegration->initialize(waylandWindow->display())) {
            delete shellIntegration;
            shellIntegration = nullptr;
            qWarning() << "KWin did not expose the input panel protocol to KDE OSK.";
            return false;
        }
    }

    waylandWindow->setShellIntegration(shellIntegration);
    return true;
}
