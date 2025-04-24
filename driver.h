#pragma once
extern "C"
{
	#include "CreateDriver.h"
}
#include "shared.h"
#include "imports.h"
#include "utils.h"
#include "Common.h"
#include "Physmem.h"


extern PVOID g_SharedSection;
HANDLE hClientEvent = NULL;
HANDLE hDriverEvent = NULL;
PKEVENT pClientEvent = NULL;
PKEVENT pDriverEvent = NULL;

BYTE* data;

VOID DriverLoop(PVOID StartContext) // 
{
	UNREFERENCED_PARAMETER(StartContext);
	BeDisableApc(true);
	auto status = CreateSharedMemory();

	if (!NT_SUCCESS(status))
	{
		CleanSharedMemory();
		return;
	}

	do
	{
		status = CreateNamedEvent(L"\\BaseNamedObjects\\KM", SynchronizationEvent, FALSE, &hDriverEvent, &pDriverEvent);
		
		if (!NT_SUCCESS(status))
		{
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
				"[-] DriverEvent creation failed!\n"));
			goto cleanUp;
		}

		status = CreateNamedEvent(L"\\BaseNamedObjects\\UM", SynchronizationEvent, FALSE, &hClientEvent, &pClientEvent);

		if (!NT_SUCCESS(status))
		{
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
				"[-] ClientEvent creation failed!\n"));;
			goto cleanUp;
		}

		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
			"[+] Events Created!\n"));

	} while (false);


	while (true)
	{
		
		status = KeWaitForSingleObject(
			pClientEvent,          // Pointer to the event object.
			Executive,       // Wait reason.
			KernelMode,      // Wait in kernel mode.
			FALSE,           // Not alertable.
			NULL             // No timeout; wait indefinitely.
		);

		if (!NT_SUCCESS(status))
		{
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
				"[-] KeWaitForSingleObject failed!\n"));
			break;
		}

		ReadSharedMemory();

		if (auto msg = (UM_Msg*)g_SharedSection)
		{
			DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_INFO_LEVEL, "[KM] Address received: 0x%llX\n", msg->address);
			if (msg->magic == MAGIC && msg->opType == OPERATION_TYPE::OP_BASE) // get process base address
			{
				KeMemoryBarrier();
				auto BaseAddr = GetProcessBaseAddress(msg->ProcId);

				if (BaseAddr == NULL)
				{
					KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
						"[-] GetProcessBaseAddress failed!\n"));
					break;
				}

				msg->address = reinterpret_cast<ULONGLONG>(BaseAddr);
				KeSetEvent(pDriverEvent, IO_NO_INCREMENT, TRUE);
			}

			if (msg->magic == MAGIC && msg->opType == OPERATION_TYPE::OP_MODULE_BASE) // get module base address(dont use it if you are only need the base address of the process (like in assaultcube))
			{
				KeMemoryBarrier();

				PEPROCESS proc;
				status = PsLookupProcessByProcessId(ULongToHandle( msg->ProcId), &proc);
				
				if (!NT_SUCCESS(status)) 
				{
					KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
						"[-] PsLookupProcessByProcessId failed!\n"));
					break;
				}

				UNICODE_STRING name;
				RtlInitUnicodeString(&name, L"client.dll"); // for cs2 ... u can change it

				PVOID module_address;
				ULONG module_size;

				status = utils::get_process_module_base(proc, &name, &module_address, &module_size);

				if (!NT_SUCCESS(status))
				{
					KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
						"[-] get_process_module_base failed!\n"));
					break;
				}

				msg->address = reinterpret_cast<ULONGLONG>(module_address);
				KeSetEvent(pDriverEvent, IO_NO_INCREMENT, TRUE);
			}

			if (msg->magic == MAGIC && msg->opType == OPERATION_TYPE::OP_READ) // read
			{
				
				KeMemoryBarrier();
			
				data = (BYTE*)ExAllocatePoolWithTag(NonPagedPool, msg->dataSize, 'bufT');
				if (!data)
				{
					KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[-] %s Failed to allocate memory\n", __FUNCTION__));
					break;
				}
				
				RtlZeroMemory(data, sizeof(data));
				SIZE_T read;
				

				status = ReadProcessMemory(msg->ProcId,
					reinterpret_cast<PVOID>(msg->address),
					data,
					msg->dataSize,
					&read);
	
				if (!NT_SUCCESS(status))
				{
					KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
						"[-] ReadProcessMemory failed\n"));

					ExFreePool(data);
					break;
				}

				RtlCopyMemory(msg->data, data, msg->dataSize);
				ExFreePool(data);

				KeSetEvent(pDriverEvent, IO_NO_INCREMENT, TRUE);
			}

			if (msg->magic == MAGIC && msg->opType == OPERATION_TYPE::OP_WRITE) // write
			{
				KeMemoryBarrier();

				if (msg->ProcId == 0 || msg->data == NULL || msg->dataSize == 0)
				{
					KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
						"[-] Some BS got provided to WriteProcessMemory\n"));
					break;
				}


				SIZE_T written = 0;
				status = WriteProcessMemory(msg->ProcId,         
					reinterpret_cast<PVOID>(msg->address),
					msg->data,           
					msg->dataSize,       
					&written);

				if (!NT_SUCCESS(status))
				{
					KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
						"[-] WriteProcessMemory failed\n"));
					break;
				}
				KeSetEvent(pDriverEvent, IO_NO_INCREMENT, TRUE);
			}

			if (msg->magic == MAGIC && msg->opType == OPERATION_TYPE::OP_EXIT) // exit
			{
				break;
			}
		}

		
	}


cleanUp:

	if (pDriverEvent)
	{
		ObDereferenceObject(pDriverEvent);
	}
	if (pClientEvent)
	{
		ObDereferenceObject(pClientEvent);
	}
	if (hClientEvent)
	{
		ZwClose(hClientEvent);
	}
	if (hDriverEvent)
	{
		ZwClose(hDriverEvent);
	}
	
	CleanSharedMemory();
	BeDisableApc(false);
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
		"[!] Thread Exited\n"));
}


