#include "ViewportTracking.h"

#include <algorithm>
#include <cmath>
#include <cwchar>
#include <cwctype>
#include <iterator>
#include <limits>

namespace
{
constexpr double kNominalFrameSeconds = 1.0 / 60.0;
constexpr double kTargetFollowAlpha = 0.25;
constexpr double kGyroUnitsPerPixel = 100000.0;
constexpr double kMaximumFrameSeconds = 0.1;

double read_double(const std::wstring& ini_path, const wchar_t* key,
    double default_value, double minimum, double maximum) noexcept
{
    wchar_t default_text[64]{};
    swprintf_s(default_text, L"%.12g", default_value);

    wchar_t value_text[128]{};
    GetPrivateProfileStringW(L"tracking", key, default_text, value_text,
        static_cast<DWORD>(std::size(value_text)), ini_path.c_str());

    wchar_t* end = nullptr;
    const double value = std::wcstod(value_text, &end);
    while (end != nullptr && std::iswspace(*end)) {
        ++end;
    }

    if (end == value_text || (end != nullptr && *end != L'\0') || !std::isfinite(value)) {
        return default_value;
    }
    return std::clamp(value, minimum, maximum);
}

bool read_bool(const std::wstring& ini_path, const wchar_t* key, bool default_value) noexcept
{
    wchar_t value_text[32]{};
    GetPrivateProfileStringW(L"tracking", key, default_value ? L"true" : L"false",
        value_text, static_cast<DWORD>(std::size(value_text)), ini_path.c_str());

    std::wstring value(value_text);
    std::transform(value.begin(), value.end(), value.begin(),
        [](wchar_t character) { return static_cast<wchar_t>(std::towlower(character)); });
    if (value == L"true" || value == L"1" || value == L"yes" || value == L"on") {
        return true;
    }
    if (value == L"false" || value == L"0" || value == L"no" || value == L"off") {
        return false;
    }
    return default_value;
}

bool write_value(const std::wstring& ini_path, const wchar_t* key,
    const std::wstring& value) noexcept
{
    return WritePrivateProfileStringW(L"tracking", key, value.c_str(), ini_path.c_str()) != FALSE;
}
}

void ViewportTrackingSettings::sanitize() noexcept
{
    if (!std::isfinite(horizontal_sensitivity)) {
        horizontal_sensitivity = 1.0;
    }
    if (!std::isfinite(vertical_sensitivity)) {
        vertical_sensitivity = 0.4;
    }
    if (!std::isfinite(smoothing)) {
        smoothing = 0.15;
    }
    if (!std::isfinite(dead_zone)) {
        dead_zone = 12.0;
    }

    horizontal_sensitivity = std::clamp(horizontal_sensitivity, 0.0, 10.0);
    vertical_sensitivity = std::clamp(vertical_sensitivity, 0.0, 10.0);
    smoothing = std::clamp(smoothing, 0.01, 1.0);
    dead_zone = std::clamp(dead_zone, 0.0, 10000.0);
}

void ViewportTrackingState::set_settings(const ViewportTrackingSettings& settings) noexcept
{
    const bool vertical_was_locked = settings_.lock_vertical;
    settings_ = settings;
    settings_.sanitize();

    if (!vertical_was_locked && settings_.lock_vertical && initialized_) {
        center_y_ = current_y_;
        pitch_offset_ = 0.0;
        target_y_ = current_y_;
        filtered_y_velocity_ = 0.0;
    }
}

const ViewportTrackingSettings& ViewportTrackingState::settings() const noexcept
{
    return settings_;
}

void ViewportTrackingState::reset(const RECT& viewport, const RECT& desktop_bounds) noexcept
{
    initialized_ = true;
    current_x_ = static_cast<double>(viewport.left);
    current_y_ = static_cast<double>(viewport.top);
    target_x_ = current_x_;
    target_y_ = current_y_;
    center_x_ = current_x_;
    center_y_ = current_y_;
    yaw_offset_ = 0.0;
    pitch_offset_ = 0.0;
    filtered_x_velocity_ = 0.0;
    filtered_y_velocity_ = 0.0;
    debug_ = {};
    clamp_state(viewport, desktop_bounds);
}

void ViewportTrackingState::recenter() noexcept
{
    if (!initialized_) {
        return;
    }

    // The current pixels stay exactly where they are. Future gyro deltas are
    // measured from this new anchor, so recentering cannot cause a jump.
    center_x_ = current_x_;
    center_y_ = current_y_;
    yaw_offset_ = 0.0;
    pitch_offset_ = 0.0;
    target_x_ = current_x_;
    target_y_ = current_y_;
    filtered_x_velocity_ = 0.0;
    filtered_y_velocity_ = 0.0;
}

RECT ViewportTrackingState::update(double gyro_x, double gyro_y, double dt_seconds,
    const RECT& viewport, const RECT& desktop_bounds) noexcept
{
    if (!initialized_) {
        reset(viewport, desktop_bounds);
    }

    if (!std::isfinite(dt_seconds) || dt_seconds <= 0.0) {
        dt_seconds = kNominalFrameSeconds;
    }
    dt_seconds = std::min(dt_seconds, kMaximumFrameSeconds);

    // Rokid X maps to pitch/vertical movement and Rokid Y maps to
    // yaw/horizontal movement in the original Vardian implementation.
    double raw_x_velocity = -(gyro_y / kGyroUnitsPerPixel) / dt_seconds;
    double raw_y_velocity = -(gyro_x / kGyroUnitsPerPixel) / dt_seconds;
    raw_x_velocity *= settings_.horizontal_sensitivity;
    raw_y_velocity *= settings_.vertical_sensitivity;

    const double raw_yaw = raw_x_velocity;
    const double raw_pitch = raw_y_velocity;

    raw_x_velocity = apply_dead_zone(raw_x_velocity, settings_.dead_zone);
    raw_y_velocity = apply_dead_zone(raw_y_velocity, settings_.dead_zone);

    const double filter_alpha = time_adjusted_alpha(settings_.smoothing, dt_seconds);
    filtered_x_velocity_ += filter_alpha * (raw_x_velocity - filtered_x_velocity_);
    filtered_y_velocity_ += filter_alpha * (raw_y_velocity - filtered_y_velocity_);

    yaw_offset_ += filtered_x_velocity_ * dt_seconds;
    target_x_ = center_x_ + yaw_offset_;

    if (settings_.lock_vertical) {
        filtered_y_velocity_ = 0.0;
        pitch_offset_ = 0.0;
        target_y_ = current_y_;
        center_y_ = current_y_;
    }
    else {
        pitch_offset_ += filtered_y_velocity_ * dt_seconds;
        target_y_ = center_y_ + pitch_offset_;
    }

    clamp_state(viewport, desktop_bounds);

    const double follow_alpha = time_adjusted_alpha(kTargetFollowAlpha, dt_seconds);
    current_x_ += (target_x_ - current_x_) * follow_alpha;
    current_y_ += (target_y_ - current_y_) * follow_alpha;
    clamp_state(viewport, desktop_bounds);

    debug_.raw_yaw = raw_yaw;
    debug_.raw_pitch = raw_pitch;
    debug_.filtered_yaw = filtered_x_velocity_;
    debug_.filtered_pitch = filtered_y_velocity_;
    debug_.target_x = target_x_;
    debug_.target_y = target_y_;
    debug_.current_x = current_x_;
    debug_.current_y = current_y_;

    const LONG width = viewport.right - viewport.left;
    const LONG height = viewport.bottom - viewport.top;
    const LONG minimum_x = desktop_bounds.left;
    const LONG minimum_y = desktop_bounds.top;
    const LONG maximum_x = std::max(minimum_x, desktop_bounds.right - width);
    const LONG maximum_y = std::max(minimum_y, desktop_bounds.bottom - height);

    RECT result{};
    result.left = std::clamp(static_cast<LONG>(std::llround(current_x_)), minimum_x, maximum_x);
    result.top = std::clamp(static_cast<LONG>(std::llround(current_y_)), minimum_y, maximum_y);
    result.right = result.left + width;
    result.bottom = result.top + height;
    return result;
}

const ViewportTrackingDebugSnapshot& ViewportTrackingState::debug_snapshot() const noexcept
{
    return debug_;
}

double ViewportTrackingState::apply_dead_zone(double value, double dead_zone) noexcept
{
    const double magnitude = std::abs(value);
    if (magnitude <= dead_zone) {
        return 0.0;
    }

    // Subtracting the threshold keeps the output continuous at the edge.
    return std::copysign(magnitude - dead_zone, value);
}

double ViewportTrackingState::time_adjusted_alpha(double nominal_alpha,
    double dt_seconds) noexcept
{
    nominal_alpha = std::clamp(nominal_alpha, 0.0, 1.0);
    if (nominal_alpha >= 1.0) {
        return 1.0;
    }
    return 1.0 - std::pow(1.0 - nominal_alpha, dt_seconds / kNominalFrameSeconds);
}

double ViewportTrackingState::clamp_coordinate(double value, double minimum,
    double maximum) noexcept
{
    if (maximum < minimum) {
        maximum = minimum;
    }
    return std::clamp(value, minimum, maximum);
}

void ViewportTrackingState::clamp_state(const RECT& viewport,
    const RECT& desktop_bounds) noexcept
{
    const double width = static_cast<double>(viewport.right - viewport.left);
    const double height = static_cast<double>(viewport.bottom - viewport.top);
    const double minimum_x = static_cast<double>(desktop_bounds.left);
    const double minimum_y = static_cast<double>(desktop_bounds.top);
    const double maximum_x = static_cast<double>(desktop_bounds.right) - width;
    const double maximum_y = static_cast<double>(desktop_bounds.bottom) - height;

    target_x_ = clamp_coordinate(target_x_, minimum_x, maximum_x);
    target_y_ = clamp_coordinate(target_y_, minimum_y, maximum_y);
    current_x_ = clamp_coordinate(current_x_, minimum_x, maximum_x);
    current_y_ = clamp_coordinate(current_y_, minimum_y, maximum_y);

    // Prevent offset wind-up and the resulting vibration when looking into an edge.
    yaw_offset_ = target_x_ - center_x_;
    pitch_offset_ = target_y_ - center_y_;
}

ViewportTrackingSettings load_viewport_tracking_settings(const std::wstring& ini_path) noexcept
{
    ViewportTrackingSettings settings;
    settings.horizontal_sensitivity = read_double(ini_path, L"horizontal_sensitivity",
        settings.horizontal_sensitivity, 0.0, 10.0);
    settings.vertical_sensitivity = read_double(ini_path, L"vertical_sensitivity",
        settings.vertical_sensitivity, 0.0, 10.0);
    settings.smoothing = read_double(ini_path, L"smoothing",
        settings.smoothing, 0.01, 1.0);
    settings.dead_zone = read_double(ini_path, L"dead_zone",
        settings.dead_zone, 0.0, 10000.0);
    settings.lock_vertical = read_bool(ini_path, L"lock_vertical", settings.lock_vertical);
    settings.sanitize();
    return settings;
}

bool save_viewport_tracking_settings(const std::wstring& ini_path,
    const ViewportTrackingSettings& input_settings) noexcept
{
    ViewportTrackingSettings settings = input_settings;
    settings.sanitize();

    wchar_t horizontal[64]{};
    wchar_t vertical[64]{};
    wchar_t smoothing[64]{};
    wchar_t dead_zone[64]{};
    swprintf_s(horizontal, L"%.6g", settings.horizontal_sensitivity);
    swprintf_s(vertical, L"%.6g", settings.vertical_sensitivity);
    swprintf_s(smoothing, L"%.6g", settings.smoothing);
    swprintf_s(dead_zone, L"%.6g", settings.dead_zone);

    bool ok = true;
    ok = write_value(ini_path, L"horizontal_sensitivity", horizontal) && ok;
    ok = write_value(ini_path, L"vertical_sensitivity", vertical) && ok;
    ok = write_value(ini_path, L"smoothing", smoothing) && ok;
    ok = write_value(ini_path, L"dead_zone", dead_zone) && ok;
    ok = write_value(ini_path, L"lock_vertical", settings.lock_vertical ? L"true" : L"false") && ok;
    return ok;
}
