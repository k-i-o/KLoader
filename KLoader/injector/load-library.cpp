#include "load-library.h"

#include <iostream>
#include "util.h"

#if defined(DISABLE_OUTPUT)
#define ILog(data, ...)
#define IPrintError(text, ...)
#else
#define ILog(text, ...) printf(text, __VA_ARGS__)
#define ILogError(text, ...) ILog(text, __VA_ARGS__); std::cerr << "Error: " << util::GetLastErrorAsString() << std::endl
#endif

bool LoadLibraryDLL(HANDLE hProc, const std::string& dllpath)
{
	HMODULE hKernel = GetModuleHandle("kernel32.dll");
	if (hKernel == NULL)
	{
		ILogError("[DLL Injection] Failed to get kernel32.dll module address.\n");
		return false;
	}

	LPVOID pLoadLibrary = (LPVOID)GetProcAddress(hKernel, "LoadLibraryA");
	if (pLoadLibrary == NULL)
	{
		ILogError("[DLL Injection] Failed to get LoadLibraryA address.\n");
		return false;
	}

	LPVOID pDLLPath = VirtualAllocEx(hProc, NULL, strlen(dllpath.c_str()) + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (pDLLPath == NULL)
	{
		ILogError("[DLL Injection] Failed to allocate memory for DLLPath in target process.\n");
		return false;
	}

	BOOL writeResult = WriteProcessMemory(hProc, pDLLPath, dllpath.c_str(), strlen(dllpath.c_str()), NULL);
	if (writeResult == FALSE)
	{
		ILogError("[DLL Injection] Failed to write remote process memory.\n");
		return false;
	}

	HANDLE hThread = CreateRemoteThread(hProc, NULL, NULL, (LPTHREAD_START_ROUTINE)pLoadLibrary, (LPVOID)pDLLPath, NULL, NULL);
	if (hThread == NULL)
	{
		ILogError("[DLL Injection] Failed to create remote thread.\n");
		VirtualFreeEx(hProc, pDLLPath, 0, MEM_RELEASE);
		return false;
	}

	if (WaitForSingleObject(hThread, 2000) == WAIT_OBJECT_0)
	{
		VirtualFreeEx(hProc, pDLLPath, 0, MEM_RELEASE);
	}

	CloseHandle(hThread);

	ILog("[DLL Injection] Successfully LoadLibraryA injection.\n");
	return true;
}