// Copyright 2019, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  C vec2 math library.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 *
 * @see xrt_vec2
 * @ingroup aux_math
 */

#pragma once

#include "xrt/xrt_defines.h"

#include "m_mathinclude.h"


#ifdef __cplusplus
extern "C" {
#endif


static inline struct xrt_vec2
m_vec2_mul(struct xrt_vec2 l, struct xrt_vec2 r)
{
	struct xrt_vec2 ret = {l.x * r.x, l.y * r.y};
	return ret;
}

static inline struct xrt_vec2
m_vec2_mul_scalar(struct xrt_vec2 l, float r)
{
	struct xrt_vec2 ret = {l.x * r, l.y * r};
	return ret;
}

static inline struct xrt_vec2
m_vec2_add(struct xrt_vec2 l, struct xrt_vec2 r)
{
	struct xrt_vec2 ret = {l.x + r.x, l.y + r.y};
	return ret;
}

static inline struct xrt_vec2
m_vec2_add_scalar(struct xrt_vec2 l, float r)
{
	struct xrt_vec2 ret = {l.x + r, l.y + r};
	return ret;
}

static inline struct xrt_vec2
m_vec2_sub(struct xrt_vec2 l, struct xrt_vec2 r)
{
	struct xrt_vec2 ret = {l.x - r.x, l.y - r.y};
	return ret;
}

static inline struct xrt_vec2
m_vec2_sub_scalar(struct xrt_vec2 l, float r)
{
	struct xrt_vec2 ret = {l.x - r, l.y - r};
	return ret;
}

static inline struct xrt_vec2
m_vec2_div(struct xrt_vec2 l, struct xrt_vec2 r)
{
	struct xrt_vec2 ret = {l.x / r.x, l.y / r.y};
	return ret;
}

static inline struct xrt_vec2
m_vec2_div_scalar(struct xrt_vec2 l, float r)
{
	struct xrt_vec2 ret = {l.x / r, l.y / r};
	return ret;
}

static inline float
m_vec2_len_sqrd(struct xrt_vec2 l)
{
	return l.x * l.x + l.y * l.y;
}


static inline float
m_vec2_len(struct xrt_vec2 l)
{
	return sqrtf(m_vec2_len_sqrd(l));
}

static inline void
m_vec2_normalize(struct xrt_vec2 *inout)
{
	*inout = m_vec2_div_scalar(*inout, m_vec2_len(*inout));
}

static inline float
m_vec2_dot(struct xrt_vec2 l, struct xrt_vec2 r)
{
	return l.x * r.x + l.y * r.y;
}

static inline struct xrt_vec2
m_vec2_lerp(struct xrt_vec2 from, struct xrt_vec2 to, float amount)
{
	// Recommend amount being in [0,1]
	return m_vec2_add(m_vec2_mul_scalar(from, 1.0f - amount), m_vec2_mul_scalar(to, amount));
}

typedef float m_vec2_float_arr[2];

static inline m_vec2_float_arr *
m_vec2_ptr_to_float_arr_ptr(struct xrt_vec2 *ptr)
{
	return (m_vec2_float_arr *)ptr;
}


#ifdef __cplusplus
}


static inline struct xrt_vec2
operator+(const struct xrt_vec2 &a, const struct xrt_vec2 &b)
{
	return m_vec2_add(a, b);
}

static inline struct xrt_vec2
operator-(const struct xrt_vec2 &a, const struct xrt_vec2 &b)
{
	return m_vec2_sub(a, b);
}

static inline struct xrt_vec2 // Until clang-format-11 is on the CI.
operator*(const struct xrt_vec2 &a, const struct xrt_vec2 &b)
{
	return m_vec2_mul(a, b);
}

static inline struct xrt_vec2
operator/(const struct xrt_vec2 &a, const struct xrt_vec2 &b)
{
	return m_vec2_div(a, b);
}

static inline void
operator+=(struct xrt_vec2 &a, const struct xrt_vec2 &b)
{
	a = a + b;
}

static inline void
operator-=(struct xrt_vec2 &a, const struct xrt_vec2 &b)
{
	a = a - b;
}

static inline void
operator*=(struct xrt_vec2 &a, const struct xrt_vec2 &b)
{
	a = a * b;
}

static inline void
operator/=(struct xrt_vec2 &a, const struct xrt_vec2 &b)
{
	a = a / b;
}


#endif
