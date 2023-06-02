/*
Bluetooth Audio Fix (BAF) - LFInteractive LLC. (c) 2020-2024
a fix for bluetooth audio in Windows 10/11
https://github.com/dcmanproductions/Bluetooth-Audio-Fix
Licensed under the GNU General Public License v3.0
https://www.gnu.org/licenses/lgpl-3.0.html
*/

#include <Windows.h>
#include <iostream>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <chrono>
#include <thread>

BOOL IsUserAdmin(VOID)
{
	BOOL b;
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID AdministratorsGroup;
	b = AllocateAndInitializeSid(
		&NtAuthority,
		2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&AdministratorsGroup);
	if (b)
	{
		if (!CheckTokenMembership(NULL, AdministratorsGroup, &b))
		{
			b = FALSE;
		}
		FreeSid(AdministratorsGroup);
	}

	return(b);
}

void SendEmptyAudioPacketToAllDevices()
{
	CoInitialize(NULL);

	IMMDeviceEnumerator* enumerator;
	IMMDeviceCollection* deviceCollection;
	IMMDevice* device;

	// Create the device enumerator
	HRESULT result = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (LPVOID*) &enumerator);
	if (FAILED(result))
	{
		// Error handling
		CoUninitialize();
		return;
	}

	// Enumerate audio render devices
	result = enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &deviceCollection);
	if (FAILED(result))
	{
		// Error handling
		enumerator->Release();
		CoUninitialize();
		return;
	}

	// Get the device count
	UINT deviceCount;
	result = deviceCollection->GetCount(&deviceCount);
	if (FAILED(result))
	{
		// Error handling
		deviceCollection->Release();
		enumerator->Release();
		CoUninitialize();
		return;
	}

	// Iterate over each audio render device
	for (UINT i = 0; i < deviceCount; i++)
	{
		// Get the device
		result = deviceCollection->Item(i, &device);
		if (FAILED(result))
		{
			// Error handling
			continue;
		}

		// Activate the device
		IAudioEndpointVolume* endpointVolume;
		result = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (LPVOID*) &endpointVolume);
		if (FAILED(result))
		{
			// Error handling
			device->Release();
			continue;
		}

		// Send an empty audio packet
		//result = endpointVolume->SetMasterVolumeLevelScalar(0.0f, NULL);
		if (FAILED(result))
		{
			// Error handling
		}

		// Release resources
		endpointVolume->Release();
		device->Release();
	}

	// Release resources
	deviceCollection->Release();
	enumerator->Release();

	CoUninitialize();
}

int main(int* length, char** args)
{
	if (!IsUserAdmin())
	{
		std::cerr << "BAF Needs to be run as administrator. Restarting as admin..." << std::endl;

		char* path = args[0];
		const WCHAR* pwcsName;
		int nChars = MultiByteToWideChar(CP_ACP, 0, path, -1, NULL, 0);
		pwcsName = new WCHAR[nChars];
		MultiByteToWideChar(CP_ACP, 0, path, -1, (LPWSTR) pwcsName, nChars);

		LPCWSTR applicationName = pwcsName;
		delete[] pwcsName;
		HINSTANCE hInstance = ShellExecute(NULL, L"runas", applicationName, NULL, NULL, SW_SHOWNORMAL);
		if ((int) hInstance <= 32)
		{
			std::cerr << "Failed to launch program as administrator" << std::endl;
			return 1;
		}
		return 1;
	}
	while (true)
	{
		std::cout << "Polling" << std::endl;
		SendEmptyAudioPacketToAllDevices();

		std::this_thread::sleep_for(std::chrono::seconds(2));
	}
	return 0;
}