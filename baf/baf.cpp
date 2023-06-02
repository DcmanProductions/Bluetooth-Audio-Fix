/*
Bluetooth Audio Fix (BAF) - LFInteractive LLC. (c) 2020-2024
a fix for bluetooth audio in Windows 10/11
https://github.com/dcmanproductions/Bluetooth-Audio-Fix
Licensed under the GNU General Public License v3.0
https://www.gnu.org/licenses/lgpl-3.0.html
*/
#include <iostream>
#include <portaudio.h>
#include <Windows.h>
#include <string>

static int paCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
	float* out = (float*) outputBuffer;
	for (unsigned int i = 0; i < framesPerBuffer; i++)
	{
		// Stereo channels are interleaved
		*out++ = 0.0f; // Left channel
		*out++ = 0.0f; // Right channel
	}
	return paContinue;
}

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

int main(int* length, char** args)
{
	if (!IsUserAdmin())
	{
		std::cerr << "BAF Needs to be run as administrator. Restarting as admin..." << std::endl;

		char* p = args[0]; //just for proper syntax highlighting ..."
		const WCHAR* pwcsName;
		// required size
		int nChars = MultiByteToWideChar(CP_ACP, 0, p, -1, NULL, 0);
		// allocate it
		pwcsName = new WCHAR[nChars];
		MultiByteToWideChar(CP_ACP, 0, p, -1, (LPWSTR) pwcsName, nChars);

		LPCWSTR applicationName = pwcsName;
		// delete it
		delete[] pwcsName;
		HINSTANCE hInstance = ShellExecute(NULL, L"runas", applicationName, NULL, NULL, SW_SHOWNORMAL);
		if ((int) hInstance <= 32)
		{
			std::cerr << "Failed to launch program as administrator" << std::endl;
			return 1;
		}
		return 1;
	}

	PaError err = Pa_Initialize();
	if (err != paNoError)
	{
		std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
	}

	PaStreamParameters inputParameters, outputParameters;
	inputParameters.device = Pa_GetDefaultInputDevice();
	inputParameters.channelCount = 2;
	inputParameters.sampleFormat = paFloat32;
	inputParameters.suggestedLatency = Pa_GetDeviceInfo(Pa_GetDefaultInputDevice())->defaultLowInputLatency;

	outputParameters.device = Pa_GetDefaultOutputDevice();
	outputParameters.channelCount = 2;
	outputParameters.sampleFormat = paFloat32;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(Pa_GetDefaultOutputDevice())->defaultLowOutputLatency;

	PaStream* stream;
	err = Pa_OpenStream(&stream, &inputParameters, &outputParameters, NULL, NULL, paClipOff, &paCallback, NULL);
	if (err != paNoError)
	{
		std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
		return 1;
	}

	err = Pa_StartStream(stream);
	if (err != paNoError)
	{
		std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
		Pa_CloseStream(stream);
		return 1;
	}

	// Run the stream for 5 seconds
	Pa_Sleep(5000);

	err = Pa_StopStream(stream);
	err = Pa_CloseStream(stream);

	Pa_Terminate();
	return 0;
}