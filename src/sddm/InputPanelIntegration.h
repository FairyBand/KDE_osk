#pragma once

#include "InputPanelRole.h"

class QWindow;

bool initializeInputPanelSurface(QWindow *window, InputPanelRole::Role role);
