#pragma once

#include <Windows.h>

#include <string>

struct ViewportTrackingSettings
{
    double horizontal_sensitivity = 1.0;
    double vertical_sensitivity = 0.4;
    double smoothing = 0.15;
    double dead_zone = 12.0;
    bool lock_vertical = false;

    void sanitize() noexcept;
};

struct ViewportTrackingDebugSnapshot
{
    double raw_yaw = 0.0;
    double raw_pitch = 0.0;
    double filtered_yaw = 0.0;
    double filtered_pitch = 0.0;
    double target_x = 0.0;
    double target_y = 0.0;
    double current_x = 0.0;
    double current_y = 0.0;
};

class ViewportTrackingState
{
public:
    void set_settings(const ViewportTrackingSettings& settings) noexcept;
    const ViewportTrackingSettings& settings() const noexcept;

    void reset(const RECT& viewport, const RECT& desktop_bounds) noexcept;
    void recenter() noexcept;

    RECT update(double gyro_x, double gyro_y, double dt_seconds,
        const RECT& viewport, const RECT& desktop_bounds) noexcept;

    const ViewportTrackingDebugSnapshot& debug_snapshot() const noexcept;

private:
    static double apply_dead_zone(double value, double dead_zone) noexcept;
    static double time_adjusted_alpha(double nominal_alpha, double dt_seconds) noexcept;
    static double clamp_coordinate(double value, double minimum, double maximum) noexcept;

    void clamp_state(const RECT& viewport, const RECT& desktop_bounds) noexcept;

    ViewportTrackingSettings settings_{};
    ViewportTrackingDebugSnapshot debug_{};

    bool initialized_ = false;
    double center_x_ = 0.0;
    double center_y_ = 0.0;
    double yaw_offset_ = 0.0;
    double pitch_offset_ = 0.0;
    double filtered_x_velocity_ = 0.0;
    double filtered_y_velocity_ = 0.0;
    double target_x_ = 0.0;
    double target_y_ = 0.0;
    double current_x_ = 0.0;
    double current_y_ = 0.0;
};

ViewportTrackingSettings load_viewport_tracking_settings(const std::wstring& ini_path) noexcept;
bool save_viewport_tracking_settings(const std::wstring& ini_path,
    const ViewportTrackingSettings& settings) noexcept;
