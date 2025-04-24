#include "Common.h"
#include "utils_um.h"
#include "main.h"
#include <conio.h>
#include "aimbot.h"
#include <cstdint>
#include "offsets.h"
#include "math.h"



const std::string process_name = "cs2.exe";

 uintptr_t base;
 uintptr_t client;

 int main()
 {
     printf("sizeof(void*): %llu\n", sizeof(void*));
     std::cin.get();
     if (OpenSharedMemory())
     {
         printf("SharedMemory Initialized: %d\n", GetLastError());
     }
     else
     {
         printf("SharedMemory Not Initialized: %d\n", GetLastError());
         clean();
         ExitSystemThread();
         return 1;
     }
     if (OpenNamedEvents())
     {
         printf("NamedEvents Initialized: %d\n", GetLastError());
     }
     else
     {
         printf("NamedEvents Not Initialized: %d\n", GetLastError());
         clean();
         ExitSystemThread();
         return 1;
     }
     
     if (base = GetBaseAddr(process_name))
     {
         printf("Base address : %p\n", base);
     }
     else
     {
         printf("Failed to get process id : %d\n", GetLastError());
         clean();
         ExitSystemThread();
         return 1;
     }

     if (client = GetModuleAddress(process_name))
     {
         printf("client : %p\n", client);
     }
     else
     {
         printf("Failed to get client : %d\n", GetLastError());
         clean();
         ExitSystemThread();
     }

     while (true) {
         
         // ur cheat logic
         Sleep(1);
     }

     return 0;
 }
