#include "Rokid.h"
#include <SetupAPI.h>
#include <hidsdi.h>
#include "helpers.h"
#include <future>

constexpr int ROKID_USB_INTERFACE_NUM = 2;
constexpr int ROKID_INTERRUPT_IN_ENDPOINT = 0x82;
constexpr int ROKID_USB_TRANSFER_TIMEOUT_MS = 1000;


Rokid::Rokid() noexcept: 
	cummulated_gyro_x(0), 
	cummulated_gyro_y(0), 
	cummulated_gyro_z(0),
	_stop(false),
	_stopped(true),
	_sensibility(10)
{
}

void Rokid::set_sensibility(const float& sensibility) noexcept {
	_sensibility = sensibility;
}

float Rokid::get_sensibility() const noexcept {
	return _sensibility;
}

Rokid::~Rokid() {
	stop();
}

#if defined(__GNUC__)
#define ROKID_PACKED __attribute__((packed))
#else
#define ROKID_PACKED
#endif /* __GNUC__ */

#if defined(_MSC_VER)
#pragma pack(push, 1)
#endif

struct rokid_usb_packed_vec
{
	float x;
	float y;
	float z;
} ROKID_PACKED;

struct rokid_usb_pkt_combined
{
	uint8_t packet_type;
	uint64_t timestamp;
	struct rokid_usb_packed_vec accel;
	struct rokid_usb_packed_vec gyro;
	struct rokid_usb_packed_vec magnetometer;
	uint8_t keys_pressed;
	uint8_t proxy_sensor;
	uint8_t _unknown_0;
	uint64_t vsync_timestamp;
	uint8_t _unknown_1[3];
	uint8_t display_brightness;
	uint8_t volume;
	uint8_t _unknown_2[3];
} ROKID_PACKED;


struct rokid_usb_pkt_sensor
{
	uint8_t packet_type;
	uint8_t sensor_type;
	uint32_t seq;
	uint8_t _unknown_0[3];
	uint64_t timestamp;
	uint8_t _unknown_1[4];
	struct rokid_usb_packed_vec vector;
	uint8_t _unknown_2[31];
} ROKID_PACKED;


void
Rokid::rokid_fusion_parse_usb_packet(const std::vector<char>& usb_buffer) noexcept
{
	struct rokid_usb_packed_vec gyro_vect = {};

	uint64_t timestamp = 0;
	bool received = false;

	switch (usb_buffer.at(0)) {
	case 4: {
		// Old-style packet, where we get one packet for each sensor
		// Order is usually the same, but not guaranteed, because packet losses.
		const struct rokid_usb_pkt_sensor* packet = (struct rokid_usb_pkt_sensor*)usb_buffer.data();
		switch (packet->sensor_type) {
		case 2: {
			gyro_vect = packet->vector;
			timestamp = packet->timestamp;
			received = true;
			break;
		}
		default: //ignore
		{}
		}
		break;
	}
	case 17: {
		// New-style combined packet
		const struct rokid_usb_pkt_combined* packet = (struct rokid_usb_pkt_combined*)usb_buffer.data();

		gyro_vect = packet->gyro;
		timestamp = packet->timestamp;
		received = true;
		break;
	}
	default: //ignore
	{}
	}

	if (received) {
		static uint64_t last_time = 0;

		if (last_time == 0) {
			// initialize last_time variable
			last_time = timestamp;
		}
		else {
			const uint64_t time_diff = timestamp - last_time;
			last_time = timestamp;

			const float actual_x = (trunc(gyro_vect.x * _sensibility) / _sensibility) *time_diff;
			const float actual_y = (trunc(gyro_vect.y * _sensibility) / _sensibility) *time_diff;
			const float actual_z = (trunc(gyro_vect.z * _sensibility) / _sensibility) *time_diff;

			//LOG_IT(LOG_INFO, "type: {}, x: {}, y {}, z {}", usb_buffer[0], actual_x, actual_y, actual_z);

			cummulated_gyro_x += actual_x;
			cummulated_gyro_y += actual_y;
			cummulated_gyro_z += actual_z;
		}
	}
}


void
Rokid::rokid_usb_thread( const std::atomic_bool& stop, std::atomic_bool& stopped)
{
	stopped = false;
	bool ok = true;
	std::vector<char> usb_buffer(ROKID_USB_BUFFER_LEN);

	_mutex.lock();
	while (!stop && ok) {
		DWORD read_length = 0;

		//
		// If NULL returned, then we cannot proceed any farther so we just exit the 
		//  the thread
		//

		_mutex.unlock();

		if (ok) {
			const BOOL readStatus = ReadFile(_usb_dev, usb_buffer.data(), ROKID_USB_BUFFER_LEN, &read_length, NULL);

			if (!readStatus)
			{
				ok = false;
			}
		}

		if (ok == true) {
			_mutex.lock();
			rokid_fusion_parse_usb_packet(usb_buffer);
			_mutex.unlock();
		}

		_mutex.lock();
	}
	_mutex.unlock();
	if (ok) {
		LOG_IT(LOG_INFO, "Usb thread exiting normally");
	}
	else {
		LOG_IT(LOG_ERROR, "Exiting on libusb error");
	}
	stopped = true;
}

WCHAR* getRokidDeviceString() {
	WCHAR* device_string = NULL;

	static WCHAR deviceBuffer[500];

	HDEVINFO hDevInfoSet = {};
	SP_DEVINFO_DATA devInfoData = {};
	SP_DEVICE_INTERFACE_DATA devIfcData = {0};
	PSP_DEVICE_INTERFACE_DETAIL_DATA devIfcDetailData = {};

	DWORD dwMemberIdx = 0, dwSize = 0, dwType = 0;
	GUID hidGuid = {};

	HidD_GetHidGuid(&hidGuid);

	LOG_IT(LOG_INFO, "HidGuid = {{{:08x}-{:04x}-{:04x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}}}\n",
		hidGuid.Data1, hidGuid.Data2, hidGuid.Data3,
		hidGuid.Data4[0], hidGuid.Data4[1], hidGuid.Data4[2], hidGuid.Data4[3],
		hidGuid.Data4[4], hidGuid.Data4[5], hidGuid.Data4[6], hidGuid.Data4[7]);

// TODO: figure out and use right format string
//	LOG_IT(LOG_INFO, "HidGuid = {{{:08lX}-{:04hX}-{:04hX}-{:02hhX}{:02hhX}-{:02hhX}{:02hhX}{:02hhX}{:02hhX}{:02hhX}{:02hhX}}}\n",
//		hidGuid.Data1, hidGuid.Data2, hidGuid.Data3,
//		hidGuid.Data4[0], hidGuid.Data4[1], hidGuid.Data4[2], hidGuid.Data4[3],
//		hidGuid.Data4[4], hidGuid.Data4[5], hidGuid.Data4[6], hidGuid.Data4[7]);

	// Retrieve a list of all present USB devices with a device interface.
	hDevInfoSet = SetupDiGetClassDevs(&hidGuid, NULL, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	LOG_IT(LOG_INFO, "hDevInfoSet Handle: {}\n", hDevInfoSet);

	if (hDevInfoSet != INVALID_HANDLE_VALUE) {

		while (true) {

			devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

			// Get SP_DEVINFO_DATA for this member.
			if (!SetupDiEnumDeviceInfo(hDevInfoSet, dwMemberIdx, &devInfoData)) {
				break;
			}

			// Get required size for device property
			SetupDiGetDeviceRegistryProperty(hDevInfoSet, &devInfoData, SPDRP_HARDWAREID, &dwType, NULL, 0, &dwSize);

			// Allocate required memory for byteArrayBuffer to hold device property.
			auto byteArrayBuffer = std::vector<BYTE>(dwSize);
			// (PBYTE)malloc(dwSize * sizeof(BYTE));

			// Get SPDRP_HARDWAREID device property
			if (SetupDiGetDeviceRegistryProperty(hDevInfoSet, &devInfoData, SPDRP_HARDWAREID, &dwType, byteArrayBuffer.data(), dwSize, NULL)) {

				// VID and PID from Rokid Max
				// #define ROKID_VID 0x04d2
				// #define ROKID_PID 0x162f
				const wchar_t* vid = L"VID_04D2";
				const wchar_t* pid = L"PID_162F";

				const wchar_t* bufferAsChar = (wchar_t*)byteArrayBuffer.data();

				LOG_IT(LOG_INFO, "byteArrayBuffer ({}): {}\n", lstrlen(bufferAsChar), ws2s(bufferAsChar).c_str());

				// Test for VID/PID
				if (bufferAsChar != NULL && wcsstr(bufferAsChar, pid) && wcsstr(bufferAsChar, vid)) {
					LOG_IT(LOG_INFO, "Found at dwMemberIdx: {}\n", dwMemberIdx);

					devIfcData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
					SetupDiEnumDeviceInterfaces(hDevInfoSet, NULL, &hidGuid, dwMemberIdx, &devIfcData);

					// Get required size for devIfcDetailData.
					SetupDiGetDeviceInterfaceDetail(hDevInfoSet, &devIfcData, NULL, 0, &dwSize, NULL);

					// Allocate required memory for devIfcDetailData.
					devIfcDetailData = (PSP_INTERFACE_DEVICE_DETAIL_DATA)malloc(dwSize);

					if (devIfcDetailData != NULL) {
						devIfcDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

						// Get devIfcDetailData
						SetupDiGetDeviceInterfaceDetail(hDevInfoSet, &devIfcData, devIfcDetailData, dwSize, &dwSize, NULL);
						LOG_IT(LOG_INFO, "DevicePath: {}\n", ws2s(devIfcDetailData->DevicePath));
						lstrcpy(deviceBuffer, devIfcDetailData->DevicePath);
						device_string = deviceBuffer;

						// Release devIfcDetailData memory
						free(devIfcDetailData);

						// we found it
						break;
					}
				}
			}

			dwMemberIdx++;
		}

	}
	else {
		LOG_IT(LOG_ERROR, "devInfo == INVALID_HANDLE_VALUE\n");
	}

	SetupDiDestroyDeviceInfoList(hDevInfoSet);

	return device_string;
}

bool
Rokid::rokid_hmd_usb_init()
{
	_usb_dev = INVALID_HANDLE_VALUE;

	const WCHAR* deviceString = getRokidDeviceString();

	if (deviceString == NULL) {
		LOG_IT(LOG_ERROR, "Failed to find Rokid Max");
		return false;
	}

	_usb_dev = CreateFile(deviceString,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,        // no SECURITY_ATTRIBUTES structure
		OPEN_EXISTING, // No special create flags
		0, //FILE_FLAG_OVERLAPPED, // We open the device as overlapped
		NULL);       // No template file

	if (INVALID_HANDLE_VALUE == _usb_dev)
	{
		const DWORD last_error = GetLastError();
		LOG_IT(LOG_ERROR, "Failed to init HID: {}", getErrorCodeDescription(last_error));
		return false;
	}


	// TODO: Read out several values
/*	struct libusb_device_descriptor usb_desc;
	res = libusb_get_device_descriptor(libusb_get_device(rokid->usb_dev), &usb_desc);
	if (res < 0) {
		ROKID_ERROR(rokid, "Failed to get descriptor: \"%s\"=>\"%s\"", libusb_error_name(res), libusb_strerror(res));
		return false;
	}

	res = libusb_get_string_descriptor_ascii(rokid->usb_dev, usb_desc.iProduct, (unsigned char *)rokid->base.str,
											 XRT_DEVICE_NAME_LEN - 1);
	if (res < 0) {
		ROKID_ERROR(rokid, "Failed to get product name: \"%s\"=>\"%s\"", libusb_error_name(res), libusb_strerror(res));
		return false;
	}
	res = libusb_get_string_descriptor_ascii(rokid->usb_dev, usb_desc.iSerialNumber,
											 (unsigned char *)rokid->base.serial, XRT_DEVICE_NAME_LEN - 1);
	if (res < 0) {
		ROKID_ERROR(rokid, "Failed to get serial: \"%s\"=>\"%s\"", libusb_error_name(res), libusb_strerror(res));
		return false;
	} */


	return true;
}


bool Rokid::start() {
	LOG_IT(LOG_INFO, "Starting Rokid driver instance");

	// This also sets base.str used below.
	if (!rokid_hmd_usb_init()) {
		return false;
	}

	if(_stopped)
	{
		_stop = false;
		_usb_thread = std::thread(&Rokid::rokid_usb_thread, this, std::ref(_stop), std::ref(_stopped)); //Create the thread and store its reference
	}

	LOG_IT(LOG_INFO, "Started Rokid driver instance");

	return true;
}

bool Rokid::stop() {
	if (!_stopped)
	{
		_stop = true;

		if (_usb_thread.joinable()) //Wait until the thread has been finished
			_usb_thread.join();
	}

	CloseHandle(_usb_dev);
	_usb_dev = INVALID_HANDLE_VALUE;

	return true;
}

bool Rokid::is_running() noexcept {
	return !_stopped;
}

void Rokid::get_gyro_angles_since_last_call(double& x, double& y, double& z) noexcept {
	x = cummulated_gyro_x;
	y = cummulated_gyro_y;
	z = cummulated_gyro_z;
	// reset values
	cummulated_gyro_x = 0;
	cummulated_gyro_y = 0;
	cummulated_gyro_z = 0;
}
