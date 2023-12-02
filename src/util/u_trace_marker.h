// Copyright 2020-2022, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Tracing support code, see @ref tracing.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_compiler.h"
#include "xrt/xrt_config_os.h"
#include "xrt/xrt_config_have.h"
#include "xrt/xrt_config_build.h"

#include <stdio.h>

#if defined(__cplusplus) && (defined(__clang__) || defined(__GNUC__))
#pragma GCC diagnostic push
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(__clang__)
#pragma GCC diagnostic ignored "-Wc++20-designator"
#endif
#endif

#if defined(XRT_FEATURE_TRACING) && defined(XRT_HAVE_PERCETTO)
#define U_TRACE_PERCETTO
#include <percetto.h>
#endif

#if !defined(XRT_FEATURE_TRACING) || !defined(XRT_HAVE_TRACY)
#define U_TRACE_FUNC_COLOR(CATEGORY, COLOR)                                                                            \
	(void)COLOR;                                                                                                   \
	U_TRACE_FUNC(CATEGORY)

#define U_TRACE_IDENT_COLOR(CATEGORY, IDENT, COLOR)                                                                    \
	(void)COLOR;                                                                                                   \
	U_TRACE_IDENT(CATEGORY, IDENT)

#define U_TRACE_BEGIN_COLOR(CATEGORY, IDENT, COLOR)                                                                    \
	(void)COLOR;                                                                                                   \
	U_TRACE_BEGIN(CATEGORY, IDENT)
#endif

#if defined(XRT_FEATURE_TRACING) && defined(XRT_HAVE_TRACY)
#ifndef TRACY_ENABLE
#define TRACY_ENABLE
#endif // TRACY_ENABLE
#define U_TRACE_TRACY
#include "tracy/TracyC.h"
#ifdef __cplusplus
#include "tracy/Tracy.hpp"
#endif
#endif


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Should the extra tracks be enabled, see @ref tracing.
 *
 * @ingroup aux_util
 */
enum u_trace_which
{
	U_TRACE_WHICH_SERVICE,
	U_TRACE_WHICH_OPENXR,
};

/*!
 * Internal setup function, use @ref U_TRACE_TARGET_SETUP, see @ref tracing.
 *
 * @ingroup aux_util
 */
void
u_trace_marker_setup(enum u_trace_which which);

/*!
 * Must be called from a non-static/global constructor context.
 *
 * @ingroup aux_util
 */
void
u_trace_marker_init(void);

#define COLOR_TRACE_MARKER(COLOR) U_TRACE_FUNC_COLOR(color, COLOR)
#define COLOR_TRACE_IDENT(IDENT, COLOR) U_TRACE_IDENT_COLOR(color, IDENT, COLOR)
#define COLOR_TRACE_BEGIN(IDENT, COLOR) U_TRACE_BEGIN_COLOR(color, IDENT, COLOR)
#define COLOR_TRACE_END(IDENT) U_TRACE_END(color, IDENT)

#define VK_TRACE_MARKER() U_TRACE_FUNC_COLOR(vk, 0xffffff)
#define VK_TRACE_IDENT(IDENT) U_TRACE_IDENT_COLOR(vk, IDENT, 0xffffff)
#define VK_TRACE_BEGIN(IDENT) U_TRACE_BEGIN_COLOR(vk, IDENT, 0xffffff)
#define VK_TRACE_END(IDENT) U_TRACE_END(vk, IDENT)

#define XRT_TRACE_MARKER() U_TRACE_FUNC_COLOR(xrt, 0x708090)
#define XRT_TRACE_IDENT(IDENT) U_TRACE_IDENT_COLOR(xrt, IDENT, 0x708090)
#define XRT_TRACE_BEGIN(IDENT) U_TRACE_BEGIN_COLOR(xrt, IDENT, 0x708090)
#define XRT_TRACE_END(IDENT) U_TRACE_END(xrt, IDENT)

#define DRV_TRACE_MARKER() U_TRACE_FUNC_COLOR(drv, 0x000080)
#define DRV_TRACE_IDENT(IDENT) U_TRACE_IDENT_COLOR(drv, IDENT, 0x000080)
#define DRV_TRACE_BEGIN(IDENT) U_TRACE_BEGIN_COLOR(drv, IDENT, 0x000080)
#define DRV_TRACE_END(IDENT) U_TRACE_END(drv, IDENT)

#define IPC_TRACE_MARKER() U_TRACE_FUNC_COLOR(ipc, 0x87cefa)
#define IPC_TRACE_IDENT(IDENT) U_TRACE_IDENT_COLOR(ipc, IDENT, 0x87cefa)
#define IPC_TRACE_BEGIN(IDENT) U_TRACE_BEGIN_COLOR(ipc, IDENT, 0x87cefa)
#define IPC_TRACE_END(IDENT) U_TRACE_END(ipc, IDENT)

#define OXR_TRACE_MARKER() U_TRACE_FUNC_COLOR(oxr, 0x7fffd4)
#define OXR_TRACE_IDENT(IDENT) U_TRACE_IDENT_COLOR(oxr, IDENT, 0x7fffd4)
#define OXR_TRACE_BEGIN(IDENT) U_TRACE_BEGIN_COLOR(oxr, IDENT, 0x7fffd4)
#define OXR_TRACE_END(IDENT) U_TRACE_END(oxr, IDENT, 0x7fffd4)

#define COMP_TRACE_MARKER() U_TRACE_FUNC_COLOR(comp, 0x00ff00)
#define COMP_TRACE_IDENT(IDENT) U_TRACE_IDENT_COLOR(comp, IDENT, 0x00ff00)
#define COMP_TRACE_BEGIN(IDENT) U_TRACE_BEGIN_COLOR(comp, IDENT, 0x00ff00)
#define COMP_TRACE_END(IDENT) U_TRACE_END(comp, IDENT)

#define SINK_TRACE_MARKER() U_TRACE_FUNC_COLOR(sink, 0xffa500)
#define SINK_TRACE_IDENT(IDENT) U_TRACE_IDENT_COLOR(sink, IDENT, 0xffa500)
#define SINK_TRACE_BEGIN(IDENT) U_TRACE_BEGIN_COLOR(sink, IDENT, 0xffa500)
#define SINK_TRACE_END(IDENT) U_TRACE_END(sink, IDENT)

#define SWAPCHAIN_TRACE_MARKER() U_TRACE_FUNC_COLOR(sc, 0x007700)
#define SWAPCHAIN_TRACE_IDENT(IDENT) U_TRACE_IDENT_COLOR(sc, IDENT, 0x007700)
#define SWAPCHAIN_TRACE_BEGIN(IDENT) U_TRACE_BEGIN_COLOR(sc, IDENT, 0x007700)
#define SWAPCHAIN_TRACE_END(IDENT) U_TRACE_END(sc, IDENT)

#define TRACK_TRACE_MARKER() U_TRACE_FUNC_COLOR(track, 0xff0000)
#define TRACK_TRACE_IDENT(IDENT) U_TRACE_IDENT_COLOR(track, IDENT, 0xff0000)
#define TRACK_TRACE_BEGIN(IDENT) U_TRACE_BEGIN_COLOR(track, IDENT, 0xff0000)
#define TRACK_TRACE_END(IDENT) U_TRACE_END(track, IDENT, 0xff0000)


/*
 *
 * When disabled.
 *
 */

#ifndef XRT_FEATURE_TRACING


#define U_TRACE_FUNC(CATEGORY)                                                                                         \
	do {                                                                                                           \
	} while (false)

#define U_TRACE_IDENT(CATEGORY, IDENT)                                                                                 \
	do {                                                                                                           \
	} while (false)

#define U_TRACE_BEGIN(CATEGORY, IDENT)                                                                                 \
	int __trace_##IDENT = 0; /* To ensure they are balanced */                                                     \
	do {                                                                                                           \
	} while (false)

#define U_TRACE_END(CATEGORY, IDENT)                                                                                   \
	do {                                                                                                           \
		(void)__trace_##IDENT; /* To ensure they are balanced */                                               \
	} while (false)

#define U_TRACE_EVENT_BEGIN_ON_TRACK(CATEGORY, TRACK, TIME, NAME)                                                      \
	do {                                                                                                           \
	} while (false)

#define U_TRACE_EVENT_BEGIN_ON_TRACK_DATA(CATEGORY, TRACK, TIME, NAME, ...)                                            \
	do {                                                                                                           \
	} while (false)

#define U_TRACE_EVENT_END_ON_TRACK(CATEGORY, TRACK, TIME)                                                              \
	do {                                                                                                           \
	} while (false)

#define U_TRACE_INSTANT_ON_TRACK(CATEGORY, TRACK, TIME, NAME)                                                          \
	do {                                                                                                           \
	} while (false)

#define U_TRACE_CATEGORY_IS_ENABLED(_) (false)

#define U_TRACE_SET_THREAD_NAME(STRING)                                                                                \
	do {                                                                                                           \
		(void)STRING;                                                                                          \
	} while (false)

/*!
 * Add to target c file to enable tracing, see @ref tracing.
 *
 * @ingroup aux_util
 */
#define U_TRACE_TARGET_SETUP(WHICH)


/*
 *
 * Tracy support.
 *
 */

#elif defined(XRT_HAVE_TRACY) // && XRT_FEATURE_TRACING

// Different wrappers for different cases.
#ifdef __cplusplus

#define U_TRACE_FUNC_COLOR(CATEGORY, COLOR) ZoneScopedC(COLOR)

#define U_TRACE_FUNC(CATEGORY) ZoneScoped

#define U_TRACE_IDENT_COLOR(CATEGORY, IDENT, COLOR) ZoneScopedNC(#IDENT, COLOR)

#define U_TRACE_IDENT(CATEGORY, IDENT) ZoneScopedN(#IDENT)

#elif !defined(XRT_OS_WINDOWS) // !__cplusplus

static inline void
u_trace_scope_cleanup(TracyCZoneCtx *ctx_ptr)
{
	TracyCZoneEnd(*ctx_ptr);
}

#define U_TRACE_FUNC_COLOR(CATEGORY, COLOR)                                                                            \
	static const struct ___tracy_source_location_data __func_loc = {                                               \
	    NULL, __func__, __FILE__, (uint32_t)__LINE__, COLOR,                                                       \
	};                                                                                                             \
	TracyCZoneCtx __attribute__((cleanup(u_trace_scope_cleanup))) ctx =                                            \
	    ___tracy_emit_zone_begin(&__func_loc, true);                                                               \
	(void)ctx


#define U_TRACE_FUNC(CATEGORY)                                                                                         \
	static const struct ___tracy_source_location_data __func_loc = {                                               \
	    NULL, __func__, __FILE__, (uint32_t)__LINE__, 0,                                                           \
	};                                                                                                             \
	TracyCZoneCtx __attribute__((cleanup(u_trace_scope_cleanup))) ctx =                                            \
	    ___tracy_emit_zone_begin(&__func_loc, true);                                                               \
	(void)ctx

#define U_TRACE_IDENT_COLOR(CATEGORY, IDENT, COLOR)                                                                    \
	static const struct ___tracy_source_location_data __##IDENT##_loc = {                                          \
	    #IDENT, __func__, __FILE__, (uint32_t)__LINE__, COLOR,                                                     \
	};                                                                                                             \
	TracyCZoneCtx __attribute__((cleanup(u_trace_scope_cleanup))) ctx##IDENT =                                     \
	    ___tracy_emit_zone_begin(&__##IDENT##_loc, true);                                                          \
	(void)ctx##IDENT

#define U_TRACE_IDENT(CATEGORY, IDENT)                                                                                 \
	static const struct ___tracy_source_location_data __##IDENT##_loc = {                                          \
	    #IDENT, __func__, __FILE__, (uint32_t)__LINE__, 0,                                                         \
	};                                                                                                             \
	TracyCZoneCtx __attribute__((cleanup(u_trace_scope_cleanup))) ctx##IDENT =                                     \
	    ___tracy_emit_zone_begin(&__##IDENT##_loc, true);                                                          \
	(void)ctx##IDENT

#else // !XRT_OS_WINDOWS && !__cplusplus
#define U_TRACE_FUNC_COLOR(CATEGORY, COLOR)                                                                            \
	do {                                                                                                           \
	} while (false)

#define U_TRACE_FUNC(CATEGORY)                                                                                         \
	do {                                                                                                           \
	} while (false)

#define U_TRACE_IDENT_COLOR(CATEGORY, IDENT, COLOR)                                                                    \
	do {                                                                                                           \
	} while (false)

#define U_TRACE_IDENT(CATEGORY, IDENT)                                                                                 \
	do {                                                                                                           \
	} while (false)

#endif // !XRT_OS_WINDOWS && !__cplusplus

#define U_TRACE_BEGIN(CATEGORY, IDENT) TracyCZoneN(__trace_##IDENT, #IDENT, true)
#define U_TRACE_BEGIN_COLOR(CATEGORY, IDENT, COLOR) TracyCZoneNC(__trace_##IDENT, #IDENT, COLOR, true)
#define U_TRACE_END(CATEGORY, IDENT) TracyCZoneEnd(__trace_##IDENT)

#define U_TRACE_EVENT_BEGIN_ON_TRACK(CATEGORY, TRACK, TIME, NAME)                                                      \
	do {                                                                                                           \
	} while (false)

#define U_TRACE_EVENT_BEGIN_ON_TRACK_DATA(CATEGORY, TRACK, TIME, NAME, ...)                                            \
	do {                                                                                                           \
	} while (false)

#define U_TRACE_EVENT_END_ON_TRACK(CATEGORY, TRACK, TIME)                                                              \
	do {                                                                                                           \
	} while (false)

#define U_TRACE_INSTANT_ON_TRACK(CATEGORY, TRACK, TIME, NAME)                                                          \
	do {                                                                                                           \
	} while (false)

#define U_TRACE_CATEGORY_IS_ENABLED(_) (true) // All categories are always enabled with Tracy.

#define U_TRACE_SET_THREAD_NAME(STRING)                                                                                \
	do {                                                                                                           \
		/* To help with thread ordering and seeing when a thread is created. */                                \
		TracyCZoneN(created, "created", true);                                                                 \
		TracyCSetThreadName(STRING);                                                                           \
		TracyCZoneEnd(created);                                                                                \
	} while (false)

#define U_TRACE_TARGET_SETUP(WHICH)


/*
 *
 * Percetto support.
 *
 */

#elif defined(XRT_HAVE_PERCETTO) // && XRT_FEATURE_TRACKING && !XRT_HAVE_TRACY

#ifndef XRT_OS_LINUX
#error "Tracing only supported on Linux"
#endif


#define U_TRACE_CATEGORIES(C, G)                                                                                       \
	C(vk, "vk")         /* Vulkan calls */                                                                         \
	C(xrt, "xrt")       /* Misc XRT calls */                                                                       \
	C(drv, "drv")       /* Driver calls */                                                                         \
	C(ipc, "ipc")       /* IPC calls */                                                                            \
	C(oxr, "st/oxr")    /* OpenXR State Tracker calls */                                                           \
	C(sink, "sink")     /* Sink/frameserver calls */                                                               \
	C(comp, "comp")     /* Compositor calls  */                                                                    \
	C(sc, "sc")         /* Swapchain calls  */                                                                     \
	C(track, "track")   /* Tracking calls  */                                                                      \
	C(timing, "timing") /* Timing calls */

PERCETTO_CATEGORY_DECLARE(U_TRACE_CATEGORIES)

PERCETTO_TRACK_DECLARE(pc_cpu);
PERCETTO_TRACK_DECLARE(pc_allotted);
PERCETTO_TRACK_DECLARE(pc_gpu);
PERCETTO_TRACK_DECLARE(pc_margin);
PERCETTO_TRACK_DECLARE(pc_error);
PERCETTO_TRACK_DECLARE(pc_info);
PERCETTO_TRACK_DECLARE(pc_present);
PERCETTO_TRACK_DECLARE(pa_cpu);
PERCETTO_TRACK_DECLARE(pa_draw);
PERCETTO_TRACK_DECLARE(pa_wait);

#define U_TRACE_FUNC(CATEGORY) TRACE_EVENT(CATEGORY, __func__)
#define U_TRACE_IDENT(CATEGORY, IDENT) TRACE_EVENT(CATEGORY, #IDENT)
#define U_TRACE_BEGIN(CATEGORY, IDENT) TRACE_EVENT_BEGIN(CATEGORY, #IDENT)
#define U_TRACE_END(CATEGORY, IDENT) TRACE_EVENT_END(CATEGORY)
#define U_TRACE_EVENT_BEGIN_ON_TRACK(CATEGORY, TRACK, TIME, NAME)                                                      \
	TRACE_EVENT_BEGIN_ON_TRACK(CATEGORY, TRACK, TIME, NAME)
#define U_TRACE_EVENT_BEGIN_ON_TRACK_DATA(CATEGORY, TRACK, TIME, NAME, ...)                                            \
	TRACE_EVENT_BEGIN_ON_TRACK_DATA(CATEGORY, TRACK, TIME, NAME, __VA_ARGS__)
#define U_TRACE_EVENT_END_ON_TRACK(CATEGORY, TRACK, TIME) TRACE_EVENT_END_ON_TRACK(CATEGORY, TRACK, TIME)
#define U_TRACE_CATEGORY_IS_ENABLED(CATEGORY) PERCETTO_CATEGORY_IS_ENABLED(CATEGORY)
#define U_TRACE_INSTANT_ON_TRACK(CATEGORY, TRACK, TIME, NAME)                                                          \
	TRACE_ANY_WITH_ARGS(PERCETTO_EVENT_INSTANT, CATEGORY, &g_percetto_track_##TRACK, TIME, NAME, 0)
#define U_TRACE_DATA(fd, type, data) u_trace_data(fd, type, (void *)&(data), sizeof(data))

#define U_TRACE_SET_THREAD_NAME(STRING)                                                                                \
	do {                                                                                                           \
		(void)STRING;                                                                                          \
	} while (false)

#define U_TRACE_TARGET_SETUP(WHICH)                                                                                    \
	void __attribute__((constructor(101))) u_trace_marker_constructor(void);                                       \
                                                                                                                       \
	void u_trace_marker_constructor(void)                                                                          \
	{                                                                                                              \
		u_trace_marker_setup(WHICH);                                                                           \
	}

#else // !XRT_FEATURE_TRACING && !XRT_HAVE_PERCETTO && !XRT_HAVE_TRACY

#error "Need to have Percetto/Perfetto"

#endif // Error checking


#ifdef __cplusplus
}
#endif

#if defined(__cplusplus) && (defined(__clang__) || defined(__GNUC__))
#pragma GCC diagnostic pop
#endif
