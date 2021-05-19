/*
 * Copyright (C) 2026 UBports Foundation
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef QTMIR_MIRTOOLKITEVENT_H
#define QTMIR_MIRTOOLKITEVENT_H

#include <mir/version.h>
#if MIR_SERVER_VERSION >= MIR_VERSION_NUMBER(2, 0, 0)
#include <miral/toolkit_event.h>
using namespace miral::toolkit;
#else
#include <mir_toolkit/event.h>
#endif

#endif // QTMIR_MIRTOOLKITEVENT_H
