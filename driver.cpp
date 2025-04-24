#include "driver.h"
#include "system.h"


// KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "DiskEnableDisableFailurePrediction (device = 0x%p) returned %08X\n", DeviceObject, DisableStatus));

NTSTATUS MainEntry(PDRIVER_OBJECT driver_object, PUNICODE_STRING  reg_path)
{
	UNREFERENCED_PARAMETER(driver_object);
	UNREFERENCED_PARAMETER(reg_path);

	create_thread();
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry()
{
	return IoCreateDriver(MainEntry);
}