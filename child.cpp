#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>

size_t recvMsg(int& err, HANDLE rd, char* b, size_t k)
{
  err = 0;
  size_t total = 0;
  while (total < k)
  {
    DWORD bytesToRead = static_cast<DWORD>(k - total);
    DWORD bytesRead = 0;
    
    BOOL success = ReadFile(rd, b + total, bytesToRead, &bytesRead, NULL);
    if (!success || bytesRead == 0)
    {
      err = -1;
      return total;
    }
    total += bytesRead;
  }
  return total;
}

int main(int argc, char** argv)
{
  if (argc != 2)
  {
    std::cerr << "Usage: child <handle>\n";
    return 1;
  }

  HANDLE hRead = NULL;
  try
  {
    uintptr_t rawHandle = std::stoull(argv[1]);
    hRead = reinterpret_cast<HANDLE>(rawHandle);
  }
  catch (...)
  {
    std::cerr << "Invalid handle argument\n";
    return 1;
  }

  if (hRead == NULL || hRead == INVALID_HANDLE_VALUE)
  {
    std::cerr << "Invalid handle value\n";
    return 1;
  }

  int err = 0;
  size_t size = 0;

  size_t n = recvMsg(err, hRead, reinterpret_cast< char* >(&size), sizeof(size));
  if (err < 0 || n != sizeof(size))
  {
    std::cerr << "Failed to read size!\n";
    CloseHandle(hRead);
    return 1;
  }

  std::vector<char> msg(size + 1);
  n = recvMsg(err, hRead, msg.data(), size);
  if (err < 0)
  {
    std::cerr << "Failed to read message!\n";
    CloseHandle(hRead);
    return 1;
  }

  msg[size] = '\0';
  std::cout << "The child process received: " << msg.data() << "\n";

  CloseHandle(hRead);
  return 0;
}
