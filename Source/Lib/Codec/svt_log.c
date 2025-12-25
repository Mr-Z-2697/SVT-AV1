/*
* Copyright(c) 2019 Intel Corporation
*
* This source code is subject to the terms of the BSD 2 Clause License and
* the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
* was not distributed with this source code in the LICENSE file, you can
* obtain it at https://www.aomedia.org/license/software-license. If the Alliance for Open
* Media Patent License 1.0 was not distributed with this source code in the
* PATENTS file, you can obtain it at https://www.aomedia.org/license/patent-license.
*/
#include "svt_log.h"
//for getenv and fopen on windows
#if defined(_WIN32) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#if !CONFIG_LOG_QUIET

static const char* log_level_str(SvtAv1LogLevel level) {
    switch (level) {
    case SVT_AV1_LOG_FATAL: return "fatal";
    case SVT_AV1_LOG_ERROR: return "error";
    case SVT_AV1_LOG_WARN: return "warn";
    case SVT_AV1_LOG_INFO: return "info";
    case SVT_AV1_LOG_DEBUG: return "debug";
    default: return "unknown";
    }
}

struct DefaultLoggerCtx {
    SvtAv1LogLevel level;
    FILE*          file;
};

static void default_logger(SvtAv1LogLevel level, const char* tag, const char* message, void* context) {
    struct DefaultLoggerCtx* ctx = context;
    if (level > ctx->level)
        return;
    if (tag)
        fprintf(ctx->file, "%s[%s]: ", tag, log_level_str(level));

    fprintf(ctx->file, "%s", message);
}

static struct DefaultLoggerCtx g_default_logger_ctx = {SVT_AV1_LOG_INFO, NULL};

void svt_log_init() {
    const char* log = getenv("SVT_LOG");
    if (log)
        g_default_logger_ctx.level = (SvtAv1LogLevel)atoi(log);

    const char* file = getenv("SVT_LOG_FILE");
    if (file)
        g_default_logger_ctx.file = fopen(file, "w+");
    else
        g_default_logger_ctx.file = stderr;
}

static SvtAv1LogCallback g_log_callback         = default_logger;
static void*             g_log_callback_context = &g_default_logger_ctx;

void svt_aom_log_set_callback(SvtAv1LogCallback callback, void* context) {
    // We only want to allow using a custom context if a custom callback is provided.
    // Otherwise the context for the default logger will be incorrect.
    // This will still allow passing NULL for context in case the callback doesn't use it.
    if (callback) {
        g_log_callback         = callback;
        g_log_callback_context = context;
    } else {
        g_log_callback         = default_logger;
        g_log_callback_context = &g_default_logger_ctx;
    }
}

void svt_log(SvtAv1LogLevel level, const char* tag, const char* format, ...) {
    char    buffer[2048];
    va_list args;
    va_start(args, format);
    int size = vsnprintf(buffer, sizeof(buffer), format, args);
    if (size < 0 || (size_t)size >= sizeof(buffer)) {
        // Message was truncated or encoding error occurred
        strcpy(buffer + sizeof(buffer) - 5, "...");
    }
    va_end(args);
    g_log_callback(level, tag, buffer, g_log_callback_context);
}

#endif //CONFIG_LOG_QUIET
