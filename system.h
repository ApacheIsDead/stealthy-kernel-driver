#pragma once
//#include <ntifs.h>


VOID create_thread() 
{
	auto status = STATUS_SUCCESS;
	HANDLE system_thread_handle;
	status = PsCreateSystemThread(&system_thread_handle, THREAD_ALL_ACCESS, NULL, NULL, NULL, DriverLoop, NULL);
	
	if (!NT_SUCCESS(status))
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
			"[-] PsCreateSystemThread failed!\n"));
		return;
	}

	PETHREAD pThread =	NULL;
	// Obtain a pointer to the thread object. ----> remove if not needed.
	status = ObReferenceObjectByHandle(system_thread_handle,
		THREAD_ALL_ACCESS,
		NULL,
		KernelMode,
		(PVOID*)&pThread,
		NULL);

	if (NT_SUCCESS(status) && pThread != NULL)
	{
		// Output the thread object's address
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
			"[+] Thread object address: %p\n", pThread));

		// Decrement the reference count when done
		ObDereferenceObject(pThread);
	}
	else
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
			"[-] ObReferenceObjectByHandle failed! Status: 0x%08X\n", status));
	}


	// Close the handle since we now have our own reference.
	ZwClose(system_thread_handle);

	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
		"[+] system thread created ! \n"));
}

