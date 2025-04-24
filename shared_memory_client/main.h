#pragma once
#include <Windows.h>
#include<stdio.h>
#include <thread>
//#include<iostream>

HANDLE h_write;
HANDLE h_read;
HANDLE hClientEvent = NULL;
HANDLE hDriverEvent = NULL;

SIZE_T DestSize = 4096;
UM_Msg* ToDriver = new UM_Msg();
UM_Msg* read_view;// read_address()
UM_Msg* write_view;// read_address()
uint32_t process_id;
void clean()
{
	SetLastError(0);

	if (hDriverEvent)
	{
		CloseHandle(hDriverEvent);
	}

	if (hClientEvent)
	{
		CloseHandle(hClientEvent);
	}

	if (h_read)
	{
		CloseHandle(h_read);
	}

	if (h_write)
	{
		CloseHandle(h_write);
	}

	if (ToDriver)
	{
		delete ToDriver;
	}

	printf("[+] Cleaned : %d\n", GetLastError());
}

UM_Msg* InitMsg(UM_Msg* msg, SIZE_T size)
{

	msg->address = NULL;
	msg->dataSize = size;
	memset(msg->data, 0, sizeof(msg->data));
	return msg;
}

bool OpenSharedMemory()
{
	 h_write = OpenFileMappingA(FILE_MAP_WRITE, false, "Global\\SharedSection");

	if (h_write == NULL || h_write == INVALID_HANDLE_VALUE)
	{
		printf("[-] failed to get handle for write: %X\n", GetLastError());
		return false;
	}

	 h_read = OpenFileMappingA(FILE_MAP_READ, false, "Global\\SharedSection");

	if (h_read == NULL || h_read == INVALID_HANDLE_VALUE)
	{
		printf("[-] failed to get handle for read: %X\n", GetLastError());
		return false;
	}

	return true;
}

bool OpenNamedEvents()
{
	hDriverEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Global\\KM");
	if (!hDriverEvent)
	{
		printf("[-] failed to open driver event: %X\n", GetLastError());
		return false;
	}
	hClientEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Global\\UM");
	if (!hClientEvent)
	{
		printf("[-] failed to open client event: %X\n", GetLastError());
		return false;
	}
	return true;
}

bool write_address(PVOID addr, const void* data, SIZE_T dataSize)
{

	// Prepare your message structure.
	UM_Msg* msg = InitMsg(ToDriver, dataSize);
	msg->address = reinterpret_cast<ULONGLONG>(addr); //***
	msg->opType = OPERATION_TYPE::OP_WRITE;
	
	printf("sizeof(void*): %llu\n", sizeof(void*));  // Should be 8 now
	printf("Address passed: 0x%llX\n", (ULONGLONG)addr);
	// Copy the provided data (of any type) into our fixed buffer.
	RtlCopyMemory(msg->data, data, dataSize);

	// Map the view for writing.
	UM_Msg* ViewBase = (UM_Msg*)MapViewOfFile(h_write, FILE_MAP_WRITE, 0, 0, DestSize);
	if (ViewBase == NULL)
	{
		printf("[-] failed to get ViewBase: %X\n", GetLastError());
		return false;
	}

	size_t len = sizeof(UM_Msg);
	if (len <= DestSize)
	{
		RtlCopyMemory(ViewBase, msg, len);
		MemoryBarrier();
		auto isSet = SetEvent(hClientEvent);
		if (!isSet)
		{
			printf("[-] %s : SetEvent failed\n", __FUNCTION__);
			return false;
		}
	}
	else
	{
		printf("[-] didn't copy memory, conditions not verified\n");
		UnmapViewOfFile(ViewBase);
		return false;
	}

	UnmapViewOfFile(ViewBase);
	return true;
}


VOID* read_address(PVOID addr, SIZE_T InSize)
{

	auto msg = InitMsg(ToDriver, InSize);
	msg->address = reinterpret_cast<ULONGLONG>(addr); //***
	msg->opType = OPERATION_TYPE::OP_READ;
	

	if (write_view)
	{
		size_t len = sizeof(UM_Msg);

		if (write_view != NULL && msg != NULL && len <= DestSize)
		{
			RtlCopyMemory(write_view, msg, len);
			MemoryBarrier();
			auto isSet = SetEvent(hClientEvent);
			if (!isSet)
			{
				printf("[-] %s : SetEvent failed\n", __FUNCTION__);
				return NULL;
			}
		}
		else
		{
			printf("[-] didnt copy memory , conditions not verified\n");
			return NULL;
		}
		if (!read_view)
		{
			read_view = (UM_Msg*)MapViewOfFile(h_read, FILE_MAP_READ, 0, 0, DestSize);
			if (read_view == NULL)
			{
				printf("[-] failed to get ReadMap: %X\n", GetLastError());
				return NULL;
			}
		}
		auto result = WaitForSingleObject(hDriverEvent, INFINITE);
		if (result == WAIT_OBJECT_0 && read_view->magic == MAGIC)
		{
			return read_view->data;
		}
		else
		{
			printf("[-] %s : WaitForSingleObject failed or magic: %X\n", __FUNCTION__, GetLastError());
			return NULL;
		}
	}

	write_view = (UM_Msg*)MapViewOfFile(h_write, FILE_MAP_WRITE, 0, 0, DestSize);

	if (write_view == NULL)
	{
		printf("[-] failed to get ViewBase: %X\n", GetLastError());
		return NULL;
	}

	size_t len = sizeof(UM_Msg);

	if (write_view != NULL && msg != NULL && len <= DestSize)
	{
		RtlCopyMemory(write_view, msg, len);
		MemoryBarrier();
		auto isSet = SetEvent(hClientEvent);
		if (!isSet)
		{
			printf("[-] %s : SetEvent failed\n", __FUNCTION__);
			return NULL;
		}
	}
	else
	{
		printf("[-] didnt copy memory , conditions not verified\n");
		return NULL;
	}
	if (!read_view)
	{
		read_view = (UM_Msg*)MapViewOfFile(h_read, FILE_MAP_READ, 0, 0, DestSize);
		if (read_view == NULL)
		{
			printf("[-] failed to get ReadMap: %X\n", GetLastError());
			return NULL;
		}
	}
	auto result = WaitForSingleObject(hDriverEvent, INFINITE);
	if (result == WAIT_OBJECT_0 && read_view->magic == MAGIC)
	{
		return read_view->data;
	}
	else
	{
		printf("[-] %s : WaitForSingleObject failed or magic: %X\n", __FUNCTION__, GetLastError());
		return NULL;
	}
}




void ExitSystemThread()
{
	SetLastError(0);
	auto cunt = InitMsg(ToDriver,0);
	cunt->opType = OPERATION_TYPE::OP_EXIT;

	auto ViewBase = (UM_Msg*)MapViewOfFile(h_write, FILE_MAP_WRITE, 0, 0, DestSize);

	if (ViewBase == NULL)
	{
		printf("[-] failed to get ViewBase: %X\n", GetLastError());
		return;
	}

	size_t len = sizeof(UM_Msg);

	if (ViewBase != NULL && cunt != NULL && len <= DestSize)
	{
		RtlCopyMemory(ViewBase, cunt, len);
		MemoryBarrier();
		auto isSet = SetEvent(hClientEvent);
		if (!isSet)
		{
			printf("[-] %s : SetEvent failed\n", __FUNCTION__);
			return;
		}
	}
	else
	{
		printf("[-] didnt copy memory , conditions not verified\n");
		return;
	}

	UnmapViewOfFile(ViewBase);
	printf("[+] ExitSystemThread : %d\n", GetLastError());
	return;
}

uintptr_t GetBaseAddr(const std::string& ProcName)
{
	 process_id = get_process_id(ProcName);
	if (!process_id)
	{
		printf("[-] failed to get process id : %X\n", GetLastError());
		return NULL;
	}

	auto pussy = InitMsg(ToDriver,0);
	pussy->ProcId = process_id;
	pussy->opType = OPERATION_TYPE::OP_BASE;

	if (read_view)
	{
		UnmapViewOfFile(read_view);
	}

	read_view = (UM_Msg*)MapViewOfFile(h_write, FILE_MAP_WRITE, 0, 0, DestSize);

	if (read_view == NULL)
	{
		printf("[-] failed to get ViewBase: %X\n", GetLastError());
		return NULL;
	}

	size_t len = sizeof(UM_Msg);

	if (read_view != NULL && pussy != NULL && len <= DestSize)
	{
		RtlCopyMemory(read_view, pussy, len);
		MemoryBarrier();
		auto isSet = SetEvent(hClientEvent);
		if (!isSet)
		{
			printf("[-] %s : SetEvent failed\n", __FUNCTION__);
			return NULL;
		}
	}
	else
	{
		printf("[-] didnt copy memory , conditions not verified\n");
		return NULL;
	}

	auto result = WaitForSingleObject(hDriverEvent, INFINITE);
	if (result == WAIT_OBJECT_0 && read_view->magic == MAGIC)
	{
		return static_cast<uintptr_t>(read_view->address); //***
	}
	else
	{
		printf("[-] %s : WaitForSingleObject failed or magic: %X\n", __FUNCTION__, GetLastError());
		return NULL;
	}

}

uintptr_t GetModuleAddress(const std::string& ProcName)
{
	process_id = get_process_id(ProcName);
	if (!process_id)
	{
		printf("[-] failed to get process id : %X\n", GetLastError());
		return NULL;
	}

	auto pussy = InitMsg(ToDriver, 0);
	pussy->ProcId = process_id;
	pussy->opType = OPERATION_TYPE::OP_MODULE_BASE;

	if (read_view)
	{
		UnmapViewOfFile(read_view);
	}

	read_view = (UM_Msg*)MapViewOfFile(h_write, FILE_MAP_WRITE, 0, 0, DestSize);

	if (read_view == NULL)
	{
		printf("[-] failed to get ViewBase: %X\n", GetLastError());
		return NULL;
	}

	size_t len = sizeof(UM_Msg);

	if (read_view != NULL && pussy != NULL && len <= DestSize)
	{
		RtlCopyMemory(read_view, pussy, len);
		MemoryBarrier();
		auto isSet = SetEvent(hClientEvent);
		if (!isSet)
		{
			printf("[-] %s : SetEvent failed\n", __FUNCTION__);
			return NULL;
		}
	}
	else
	{
		printf("[-] didnt copy memory , conditions not verified\n");
		return NULL;
	}

	auto result = WaitForSingleObject(hDriverEvent, INFINITE);
	if (result == WAIT_OBJECT_0 && read_view->magic == MAGIC)
	{
		return static_cast<uintptr_t>(read_view->address); //***
	}
	else
	{
		printf("[-] %s : WaitForSingleObject failed or magic: %X\n", __FUNCTION__, GetLastError());
		return NULL;
	}

}

template <typename T>
T read(const uintptr_t addr)
{
	T* value{};
	value = reinterpret_cast<T*>(read_address(reinterpret_cast<PVOID>(addr), sizeof(T)));
	if (!value)
	{
		printf("Failed to read memory.\n");
		clean();
		exit(1);
	}
	return *value;
}


template <typename T>
void write(uintptr_t addr, const T& value)
{
	write_address(reinterpret_cast<PVOID>(addr), &value, sizeof(T));
}

// reduce remapping


