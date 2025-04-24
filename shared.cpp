#include "shared.h"
//#include <sddl.h>



HANDLE g_SectionHandle;
PVOID g_SectionPointer;
PVOID g_SharedSection{nullptr};
SIZE_T SharedMemSize = 1024 * 10;

extern "C" POBJECT_TYPE* MmSectionObjectType;


NTSTATUS CreateSharedMemory()
{
	auto status = STATUS_SUCCESS;

	SECURITY_DESCRIPTOR SecDes;
	status = RtlCreateSecurityDescriptor(&SecDes, SECURITY_DESCRIPTOR_REVISION);
	
	if (!NT_SUCCESS(status))
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
			"[-] failed creating security descriptor\n"));
		return status;
	}

	auto Dacl_lenght = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) * 3 +
		RtlLengthSid(SeExports->SeAliasAdminsSid) +
		RtlLengthSid(SeExports->SeLocalSystemSid) +
		RtlLengthSid(SeExports->SeWorldSid);

	auto Dacl = ExAllocatePoolWithTag(PagedPool, Dacl_lenght, 'lcaD');

	if (Dacl == NULL)
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
			"[-] failed allocating memory for Dacl\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	status = RtlCreateAcl((PACL)Dacl, Dacl_lenght, ACL_REVISION);

	if (!NT_SUCCESS(status))
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
			"[-] failed creating Acl\n"));
		ExFreePool(Dacl);
		return status;
	}

	status = RtlAddAccessAllowedAce((PACL)Dacl, ACL_REVISION, FILE_ALL_ACCESS, SeExports->SeAliasAdminsSid);

	if (!NT_SUCCESS(status))
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
			"[-] failed adding access to AliesAdminSid\n"));
		ExFreePool(Dacl);
		return status;
	}

	status = RtlAddAccessAllowedAce((PACL)Dacl, ACL_REVISION, FILE_ALL_ACCESS, SeExports->SeLocalSystemSid);

	if (!NT_SUCCESS(status))
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
			"[-] failed adding access to LocalSystemSid\n"));
		ExFreePool(Dacl);
		return status;
	}

	status = RtlAddAccessAllowedAce((PACL)Dacl, ACL_REVISION, FILE_ALL_ACCESS, SeExports->SeWorldSid);

	if (!NT_SUCCESS(status))
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
			"[-] failed adding access to SeWorldSid\n"));
		ExFreePool(Dacl);
		return status;
	}

	status = RtlSetDaclSecurityDescriptor(&SecDes, TRUE, (PACL)Dacl, FALSE);

	if (!NT_SUCCESS(status))
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
			"[-] failed Setting DaclSecurityDescriptor\n"));
		ExFreePool(Dacl);
		return status;
	}

	OBJECT_ATTRIBUTES OA;
	UNICODE_STRING SectionName;
	RtlInitUnicodeString(&SectionName, L"\\BaseNamedObjects\\SharedSection");

	InitializeObjectAttributes(&OA, &SectionName, OBJ_CASE_INSENSITIVE, NULL, &SecDes);

	LARGE_INTEGER MaxSize{};
	MaxSize.HighPart = 0;
	MaxSize.LowPart = 1024 * 10;

	status = ZwCreateSection(&g_SectionHandle, SECTION_ALL_ACCESS, &OA, &MaxSize, // change to MmCreateSection - optional
		PAGE_READWRITE,
		SEC_COMMIT| SEC_NOCACHE,
		NULL);

	if (!NT_SUCCESS(status))
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
			"[-] failed Creaintg Section\n"));
		ExFreePool(Dacl);
		return status;
	}

	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
		"[+] Section Object Created !\n"));

	status = ObReferenceObjectByHandle(g_SectionHandle,
		SECTION_ALL_ACCESS,
		*MmSectionObjectType, KernelMode,
		&g_SectionPointer, NULL);

	if (!NT_SUCCESS(status))
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
			"[-] %s : failed to get section pointer\n", __FUNCTION__));
		ExFreePool(Dacl);
		return status;
	}

	ExFreePool(Dacl);
	return status;
}

void ReadSharedMemory()
{
	auto status = STATUS_SUCCESS;
	
	if (! g_SectionHandle)
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
			"[-] Section handle is invalid !(%s)\n", __FUNCTION__));
		return;
	}

	if (g_SharedSection)
	{
		return;
	}

	//SIZE_T ViewSize = SharedMemSize;

	status = MmMapViewInSystemSpace(g_SectionPointer, &g_SharedSection, &SharedMemSize);

	if (!NT_SUCCESS(status))
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
			"[-] failed to map view of section !\n"));
		ZwClose(g_SectionHandle);
		return;
	}
}




void CleanSharedMemory()
{
	
	
	if (g_SharedSection)
	{
		auto status = MmUnmapViewInSystemSpace(g_SharedSection); 
		if (!NT_SUCCESS(status))
		{
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
				"[-] %s : failed to unmap view of section !\n", __FUNCTION__));
			return;
		}
	}
	
	if(g_SectionPointer)
	{
		ObDereferenceObject(g_SectionPointer);
	}
	
	if (g_SectionHandle)
	{
		ZwClose(g_SectionHandle);
	}
}

NTSTATUS CreateNamedEvent(_In_ PCWSTR EventName, _In_ EVENT_TYPE Type, _In_ BOOLEAN InitialState,_Out_ PHANDLE EventHandle, _Out_ PKEVENT* ppEvent)
{
	
	if (ppEvent == NULL)
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
			"[-] %s : ppEvent is NULL !\n", __FUNCTION__));
		return STATUS_INVALID_PARAMETER;
	}

	NTSTATUS status;
	UNICODE_STRING uniEventName;
	OBJECT_ATTRIBUTES objAttr;
	PSECURITY_DESCRIPTOR pSecDescriptor;
	UCHAR secDescriptorBuffer[SECURITY_DESCRIPTOR_MIN_LENGTH];

	// Point pSecDescriptor to your buffer (or allocate dynamically)
	pSecDescriptor = (PSECURITY_DESCRIPTOR)&secDescriptorBuffer;

	// Initialize the security descriptor
	status = RtlCreateSecurityDescriptor(pSecDescriptor, SECURITY_DESCRIPTOR_REVISION);
	if (!NT_SUCCESS(status))
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
			"[-] %s : RtlCreateSecurityDescriptor failed !\n", __FUNCTION__));
		return status;
	}

	// Manually build the DACL.
	// In this example we grant full access to everyone. In a real scenario,
	// you should build a proper ACL that meets your security requirements.
	status = RtlSetDaclSecurityDescriptor(
		pSecDescriptor,  // Security descriptor pointer
		TRUE,            // Dacl present
		NULL,            // NULL DACL means full access for everyone.
		FALSE            // Not defaulted
	);
	if (!NT_SUCCESS(status))
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
			"[-] %s : RtlSetDaclSecurityDescriptor failed !\n", __FUNCTION__));
		return status;
	}

	// Initialize the event name as a UNICODE_STRING
	RtlInitUnicodeString(&uniEventName, EventName);

	// Initialize object attributes with the manually built security descriptor
	InitializeObjectAttributes(
		&objAttr,
		&uniEventName,
		OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
		NULL,             // No root directory is needed here.
		pSecDescriptor    // Manually built security descriptor
	);

	// Create the event object.
	status = ZwCreateEvent(
		EventHandle,       // Handle for the created event
		EVENT_ALL_ACCESS,  // Desired access mask
		&objAttr,          // Object attributes initialized above
		Type,              // Event type: NotificationEvent or SynchronizationEvent
		InitialState       // Initial state (TRUE for signaled, FALSE otherwise)
	);

	if (!NT_SUCCESS(status))
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
			"[-] %s : ZwCreateEvent failed !\n", __FUNCTION__));
		return status;
	}



	status = ObReferenceObjectByHandle(
		*EventHandle,                           // The event handle
		EVENT_ALL_ACCESS,               // Desired access (adjust as needed)
		*ExEventObjectType,               // The event object type
		KernelMode,                       // Access mode in the call (use KernelMode when in driver)
		(PVOID*)ppEvent,                 // Returns the pointer to the event object (cast appropriately)
		NULL                              // Optionally, pointer to OBJECT_HANDLE_INFORMATION (not used here)
	);

	if (!NT_SUCCESS(status))
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
			"[-] %s : ObReferenceObjectByHandle failed !\n", __FUNCTION__));
		ZwClose(EventHandle);
		return status;
	}

	return status;
}
