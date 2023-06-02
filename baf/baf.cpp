/*
Bluetooth Audio Fix (BAF) - LFInteractive LLC. (c) 2020-2024
a fix for bluetooth audio in Windows 10/11
https://github.com/dcmanproductions/Bluetooth-Audio-Fix
Licensed under the GNU General Public License v3.0
https://www.gnu.org/licenses/lgpl-3.0.html
*/

#include <Windows.h>
#include <iostream>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <chrono>
#include <thread>
#include <cmath>

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

const double PI = 3.14159265358979323846;
void SendEmptyAudioPacketToAllDevices()
{
	CoInitialize(nullptr);

	IMMDeviceEnumerator* enumerator = nullptr;
	IMMDeviceCollection* deviceCollection = nullptr;
	IMMDevice* device = nullptr;
	IAudioClient* audioClient = nullptr;
	IAudioRenderClient* renderClient = nullptr;

	HRESULT result = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), reinterpret_cast<LPVOID*>(&enumerator));
	if (FAILED(result))
	{
		// Error handling
		CoUninitialize();
		return;
	}

	result = enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &deviceCollection);
	if (FAILED(result))
	{
		// Error handling
		enumerator->Release();
		CoUninitialize();
		return;
	}

	UINT deviceCount = 0;
	result = deviceCollection->GetCount(&deviceCount);
	if (FAILED(result))
	{
		// Error handling
		deviceCollection->Release();
		enumerator->Release();
		CoUninitialize();
		return;
	}

	for (UINT i = 0; i < deviceCount; i++)
	{
		result = deviceCollection->Item(i, &device);
		if (FAILED(result))
		{
			// Error handling
			continue;
		}

		result = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&audioClient));
		if (FAILED(result))
		{
			// Error handling
			device->Release();
			continue;
		}

		WAVEFORMATEX* mixFormat = nullptr;
		result = audioClient->GetMixFormat(&mixFormat);
		if (FAILED(result))
		{
			// Error handling
			audioClient->Release();
			device->Release();
			continue;
		}

		// Set the audio format to match the mix format
		result = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 0, 0, mixFormat, nullptr);
		if (FAILED(result))
		{
			// Error handling
			CoTaskMemFree(mixFormat);
			audioClient->Release();
			device->Release();
			continue;
		}

		CoTaskMemFree(mixFormat);

		result = audioClient->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(&renderClient));
		if (FAILED(result))
		{
			// Error handling
			audioClient->Release();
			device->Release();
			continue;
		}

		// Get the buffer size
		UINT32 bufferSize = 0;
		result = audioClient->GetBufferSize(&bufferSize);
		if (FAILED(result))
		{
			// Error handling
			renderClient->Release();
			audioClient->Release();
			device->Release();
			continue;
		}

		// Fill the buffer with silent audio data
		//BYTE* buffer = nullptr;
		//result = renderClient->GetBuffer(bufferSize, &buffer);
		//if (FAILED(result))
		//{
		//	// Error handling
		//	renderClient->Release();
		//	audioClient->Release();
		//	device->Release();
		//	continue;
		//}

		//memset(buffer, 0, bufferSize);

		// Generate a sine wave as audio data
		BYTE* buffer = nullptr;
		result = renderClient->GetBuffer(bufferSize, reinterpret_cast<BYTE**>(&buffer));
		if (FAILED(result))
		{
			// Error handling
			renderClient->Release();
			audioClient->Release();
			device->Release();
			continue;
		}

		double frequency = 440.0;  // Frequency of the sine wave (440 Hz for A4)
		double amplitude = 0.3;    // Amplitude of the sine wave (adjust as desired)
		double time = 0.0;         // Current time
		double angularFrequency = 2.0 * PI * frequency;

		for (UINT32 i = 0; i < bufferSize; i += mixFormat->nBlockAlign)
		{
			// Generate sample for each channel (assuming 2 channels)
			for (UINT32 channel = 0; channel < mixFormat->nChannels; channel++)
			{
				// Calculate the sine wave sample
				double sampleValue = amplitude * sin(angularFrequency * time);

				// Convert the sample to the appropriate format (16-bit signed integer)
				INT16 sample = static_cast<INT16>(sampleValue * 32767.0);

				// Write the sample to the buffer (assuming 16-bit audio)
				*reinterpret_cast<INT16*>(buffer + i + (channel * sizeof(INT16))) = sample;
			}

			// Update the time based on the sample rate
			time += 200;
		}

		// Release the buffer
		result = renderClient->ReleaseBuffer(bufferSize, 0);
		if (FAILED(result))
		{
			// Error handling
		}

		renderClient->Release();
		audioClient->Release();
		device->Release();

		// Release the buffer
		result = renderClient->ReleaseBuffer(bufferSize, 0);
		if (FAILED(result))
		{
			// Error handling
		}

		renderClient->Release();
		audioClient->Release();
		device->Release();
	}

	deviceCollection->Release();
	enumerator->Release();

	CoUninitialize();
}

int main(int* length, char** args)
{
	if (!IsUserAdmin())
	{
		std::cerr << "BAF Needs to be run as administrator. Attempting to restarting as administrator..." << std::endl;

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