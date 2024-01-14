#pragma once
#include <mutex>
#include <future>
#include <thread>
#include <Windows.h>

class Rokid
{
	static const int ROKID_USB_BUFFER_LEN = 0x40;

	// cummulated values for x angle since last request
	double cummulated_gyro_x;
	// cummulated values for y angle since last request
	double cummulated_gyro_y;
	// cummulated values for z angle since last request
	double cummulated_gyro_z;

	std::mutex _mutex;

	void rokid_usb_thread(std::atomic_bool& stop, std::atomic_bool& stopped);

	std::thread _usb_thread; 

	bool rokid_hmd_usb_init();

	HANDLE _usb_dev = INVALID_HANDLE_VALUE;

	// to stop the thread
	std::atomic_bool _stop;
	std::atomic_bool _stopped;

	void rokid_fusion_parse_usb_packet(unsigned char usb_buffer[ROKID_USB_BUFFER_LEN]);

	float _sensibility;

public:
	Rokid();

	~Rokid();


	bool start();

	bool stop();

	bool is_running();

	void get_gyro_angles_since_last_call(double &x, double& y, double& z);

	void set_sensibility(const float& sensibility);

	float get_sensibility();
};

