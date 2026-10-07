// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 the DPF Sentinel project
#include "dpf_monitor.h"
#include <math.h>
#include "../config.h"

bool isDpfIdleBlockageFlagged(float diffPressureHpa, float rpm) {
    if (isnan(diffPressureHpa) || isnan(rpm)) return false;
    return rpm <= DPF_IDLE_RPM_CEILING &&
           diffPressureHpa >= DPF_IDLE_PRESSURE_THRESHOLD_HPA;
}
