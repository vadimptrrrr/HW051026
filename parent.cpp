#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdint>

size_t sendMsg(int& err, HANDLE wr, const char* b, size_t k)
{
  err = 0;
  DWORD bytesWritten = 0;

  BOOL success = WriteFile(wr, &k, sizeof(k), &bytesWritten, NULL);
  if (!success || bytesWritten != sizeof(k))
  {
    err = -1;
    return 0;
  }

  size_t total = 0;
  while (total < k)
  {
    DWORD bytesToSend = static_cast<DWORD>(k - total);
    success = WriteFile(wr, b + total, bytesToSend, &bytesWritten, NULL);
    if (!success || bytesWritten == 0)
    {
      err = -1;
      return total;
    }
    total += bytesWritten;
  }
  return total;
}

int main()
{
  SECURITY_ATTRIBUTES saAttr;
  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = NULL;

  HANDLE hRead = NULL;
  HANDLE hWrite = NULL;

  if (!CreatePipe(&hRead, &hWrite, &saAttr, 0))
  {
    std::cerr << "CreatePipe failed (" << GetLastError() << ")\n";
    return 1;
  }

  if (!SetHandleInformation(hWrite, HANDLE_FLAG_INHERIT, 0))
  {
    std::cerr << "SetHandleInformation failed (" << GetLastError() << ")\n";
    CloseHandle(hRead);
    CloseHandle(hWrite);
    return 1;
  }

  uintptr_t rawReadHandle = reinterpret_cast< uintptr_t >(hRead);
  std::string cmdLine = "child.exe " + std::to_string(rawReadHandle);

  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(STARTUPINFOA));
  si.cb = sizeof(STARTUPINFOA);
  ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

  std::vector< char > cmdBuffer(cmdLine.begin(), cmdLine.end());
  cmdBuffer.push_back('\0');

  if (!CreateProcessA(
        NULL,
        cmdBuffer.data(),
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &si,
        &pi))
  {
    std::cerr << "CreateProcess failed (" << GetLastError() << ")\n";
    CloseHandle(hRead);
    CloseHandle(hWrite);
    return 1;
  }

  CloseHandle(hRead);

  std::string msg;
  if (std::getline(std::cin, msg))
  {
    int err = 0;
    sendMsg(err, hWrite, msg.c_str(), msg.size());
    if (err < 0)
    {
      std::cerr << "Error sending data to child!\n";
    }
  }

  CloseHandle(hWrite);

  WaitForSingleObject(pi.hProcess, INFINITE);

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  return 0;
}
