/*
 * Copyright (C) 2017-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "runtime/helpers/abort.h"

#define UNRECOVERABLE_IF(expression)                   \
                                                       \
    if (expression) {                                  \
        OCLRT::abortUnrecoverable(__LINE__, __FILE__); \
    }

#define UNREACHABLE(...) std::abort()

#ifndef DEBUG_BREAK_IF
#ifdef _DEBUG
#define DEBUG_BREAK_IF(expression)             \
                                               \
    if (expression) {                          \
        OCLRT::debugBreak(__LINE__, __FILE__); \
    }
#else
#define DEBUG_BREAK_IF(expression) (void)0
#endif // _DEBUG
#endif // !DEBUG_BREAK_IF

namespace OCLRT {
void debugBreak(int line, const char *file);
[[noreturn]] void abortUnrecoverable(int line, const char *file);
} // namespace OCLRT
