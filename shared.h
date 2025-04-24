#pragma once
#include <ntifs.h>


NTSTATUS CreateSharedMemory();
void ReadSharedMemory();
NTSTATUS CreateNamedEvent(_In_ PCWSTR EventName, _In_ EVENT_TYPE Type, _In_ BOOLEAN InitialState, _Out_ PHANDLE EventHandle, _Out_ PKEVENT* ppEvent);
void CleanSharedMemory();
