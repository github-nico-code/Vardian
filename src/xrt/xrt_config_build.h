// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  What internal components we are building.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup xrt_iface
 */

#pragma once


/* keep sorted */

#define XRT_MODULE_AUX_VIVE
#define XRT_MODULE_COMPOSITOR
#define XRT_MODULE_COMPOSITOR_MAIN
#define XRT_MODULE_COMPOSITOR_NULL
/* #undef XRT_MODULE_MERCURY_HANDTRACKING */
#define XRT_MODULE_IPC
#define XRT_MODULE_MONADO_GUI
#define XRT_MODULE_MONADO_CLI


/* keep sorted */

/* #undef XRT_FEATURE_CLIENT_DEBUG_GUI */
/* #undef XRT_FEATURE_COLOR_LOG */
#define XRT_FEATURE_DEBUG_GUI
#define XRT_FEATURE_OPENXR
/* #undef XRT_FEATURE_OPENXR_DEBUG_UTILS */
#define XRT_FEATURE_OPENXR_LAYER_CUBE
#define XRT_FEATURE_OPENXR_LAYER_CYLINDER
#define XRT_FEATURE_OPENXR_LAYER_DEPTH
#define XRT_FEATURE_OPENXR_LAYER_EQUIRECT1
#define XRT_FEATURE_OPENXR_LAYER_EQUIRECT2
#define XRT_FEATURE_RENDERDOC
#define XRT_FEATURE_SERVICE
/* #undef XRT_FEATURE_SERVICE_SYSTEMD */
/* #undef XRT_FEATURE_SLAM */
#define XRT_FEATURE_SSE2
#define XRT_FEATURE_STEAMVR_PLUGIN
/* #undef XRT_FEATURE_TRACING */
#define XRT_FEATURE_WINDOW_PEEK


/* keep sorted */

#define XRT_IPC_MSG_SOCK_FILENAME "monado_comp_ipc"
#define XRT_IPC_SERVICE_PID_FILENAME "monado.pid"
