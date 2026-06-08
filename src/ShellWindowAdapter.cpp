#include "ShellWindowAdapter.h"

#include <QDebug>
#include <QMargins>
#include <QString>
#include <QWindow>
#include <QtGlobal>

#ifdef HAVE_LAYER_SHELL_QT
#include <LayerShellQt/Shell>
#include <LayerShellQt/Window>
#endif

namespace
{
bool isWaylandSession()
{
    return !qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY");
}
}

ShellWindowAdapter::ShellWindowAdapter(QObject *parent)
    : QObject(parent)
#ifdef HAVE_LAYER_SHELL_QT
    , m_available(isWaylandSession())
#endif
{
}

void ShellWindowAdapter::prepareEnvironment()
{
#ifdef HAVE_LAYER_SHELL_QT
    if (isWaylandSession()) {
        LayerShellQt::Shell::useLayerShell();
    }
#endif
}

bool ShellWindowAdapter::available() const
{
    return m_available;
}

void ShellWindowAdapter::configure(QObject *windowObject,
                                   const QString &mode,
                                   int floatingX,
                                   int floatingY,
                                   int width,
                                   int height)
{
#ifndef HAVE_LAYER_SHELL_QT
    Q_UNUSED(windowObject)
    Q_UNUSED(mode)
    Q_UNUSED(floatingX)
    Q_UNUSED(floatingY)
    Q_UNUSED(width)
    Q_UNUSED(height)

    if (!m_warned) {
        qWarning() << "LayerShellQt was not available at build time; Wayland positioning and focus avoidance are limited.";
        m_warned = true;
    }
#else
    QWindow *window = qobject_cast<QWindow *>(windowObject);
    if (window == nullptr) {
        if (!m_warned) {
            qWarning() << "Cannot configure layer-shell placement: root object is not a QWindow.";
            m_warned = true;
        }
        return;
    }

    LayerShellQt::Window *layerWindow = LayerShellQt::Window::get(window);
    if (layerWindow == nullptr) {
        if (!m_warned) {
            qWarning() << "Cannot configure layer-shell placement: LayerShellQt did not return a window handle.";
            m_warned = true;
        }
        return;
    }

    layerWindow->setScope(QStringLiteral("kde-osk"));
    layerWindow->setLayer(LayerShellQt::Window::LayerOverlay);
    layerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
    layerWindow->setExclusiveZone(0);

    Q_UNUSED(width)
    Q_UNUSED(height)

    if (mode == QStringLiteral("dockTop")) {
        layerWindow->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop)
                                | LayerShellQt::Window::AnchorLeft
                                | LayerShellQt::Window::AnchorRight);
        layerWindow->setMargins(QMargins());
        return;
    }

    if (mode == QStringLiteral("float")) {
        layerWindow->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop)
                                | LayerShellQt::Window::AnchorLeft);
        layerWindow->setMargins(QMargins(qMax(0, floatingX), qMax(0, floatingY), 0, 0));
        return;
    }

    layerWindow->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorBottom)
                            | LayerShellQt::Window::AnchorLeft
                            | LayerShellQt::Window::AnchorRight);
    layerWindow->setMargins(QMargins());
#endif
}

void ShellWindowAdapter::moveFloating(QObject *windowObject, int floatingX, int floatingY)
{
    QWindow *window = qobject_cast<QWindow *>(windowObject);
    if (window == nullptr) {
        return;
    }

    window->setPosition(qMax(0, floatingX), qMax(0, floatingY));
}
