#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <iostream>
#include <conio.h>
#include <TlHelp32.h>

int argc = {};
char** argv = {};
HANDLE gameHandle = {};



void LOG(const char* msg)
{
	std::cerr << "[ERROR]: " << msg << "\n" << "press any key to exit...\n";
	_getch();
	
}

bool GetProcess() //detect iw3mp.exe and get a handle to it
{
	if (argc != 2)
	{
		LOG("You must supply 1 dll as an argument. Slide the .dll file on the executable file");
		return false;
	}
	const char* path = argv[1];
	
	const size_t length = strlen(path);
	
	for (size_t i = 0; i < length; i++)
	{
		if (argv[1][i] != '.')
			continue;

		else //dot detected
		{
			const char* temp = &path[i];

			if (strcmp(temp, ".dll") != 0) //not equal
			{
				LOG("The provided file is not a .dll");
				return false;
			}
			break;
			
		}
	}
	//dll valid, find the iw3mp.exe process

	HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (handle == INVALID_HANDLE_VALUE)
	{
		LOG("CreateToolhelp32Snapshot failed");
		return false;
	}

	PROCESSENTRY32 info = {};
	info.dwSize = sizeof(PROCESSENTRY32);

	if (Process32First(handle, &info))
	{
		//skip the first one because it's invalid

		while (Process32Next(handle, &info))
		{
			if (strcmp(info.szExeFile, "iw3mp.exe") == 0) //if found
			{
				gameHandle = OpenProcess(PROCESS_ALL_ACCESS, 0, info.th32ProcessID);
				std::cout << info.szExeFile << " pID: " << info.th32ProcessID << "\n";
				return true;
			}
		}
		CloseHandle(handle);

		LOG("Couldn't get game's handle");
		return false;
	
	}
	else
	{
		LOG("Process32First failed");
		return false;
	}
	
	return false;

}

bool inject()
{
	//create a remote thread in iw3mp.exe, and pass LoadLibraryA adress as the function, with the dllPath as argument
	//since the new thread can't straightforwardly access the injector's data, dllPath should be written to the memory of the game, where it can be accessed easily by the new thread

	const char* dllPath = argv[1];
	const size_t pathLength = strlen(dllPath) + 1; //consider the null terminator

	void* pathAddress = VirtualAllocEx(gameHandle, NULL, pathLength, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READ);
	if (pathAddress == nullptr)
	{
		LOG("Couldn't allocate memory in the process");
		return false;
	}

	SIZE_T bytesWritten = {};
	bool succeded = WriteProcessMemory(gameHandle, pathAddress, dllPath, pathLength, &bytesWritten);

	if (!succeded || bytesWritten != pathLength)
	{
		LOG("Failed to write dllPath to the process");
		return false;
	}

	HANDLE thread = CreateRemoteThread(gameHandle, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, pathAddress, NULL, NULL);

	
	if (FAILED(thread))
	{
		LOG("Couldn't create a remote thread in the process");
		return false;
	}

	WaitForSingleObject(thread, 1); // wait for the thread to exit, because otherwise the game will crash when trying to VirtualFreeEx()

	bool aa = VirtualFreeEx(gameHandle, pathAddress, NULL, MEM_RELEASE);
	if (!aa)
	{ 
		LOG("Failed to release");
		return false;
	}
	CloseHandle(gameHandle);
	return true;
}

int main(int argc1, char** argv1)
{
	argc = argc1;
	argv = argv1;

	if (GetProcess())
	{
		if (inject() == false)
		{
			LOG("Failed to inject");
			return -1;
		}
		std::cout << "Injected\n";
		MessageBeep(0);
		Sleep(2000);
		return 0;
		
	}
}
