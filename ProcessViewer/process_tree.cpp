#include "stdafx.h"

bool set_privilege(_In_z_ const wchar_t* privilege, _In_ bool enable)
{
	if (IsWindowsXPOrGreater())
	{
		HANDLE hToken;
		if (TRUE != OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken))
		{
			if (GetLastError() == ERROR_NO_TOKEN)
			{
				if (ImpersonateSelf(SecurityImpersonation) != TRUE)
				{
					printf("ImpersonateSelf( ) failed. gle=0x%08x", GetLastError());
					return false;
				}

				if (TRUE != OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken))
				{
					printf("OpenThreadToken() failed. gle=0x%08x", GetLastError());
					return false;
				}
			}
			else
			{
				printf("OpenThread() failed. gle=0x%08x", GetLastError());
				return false;
			}
		}

		TOKEN_PRIVILEGES tp = { 0 };
		LUID luid = { 0 };
		DWORD cb = sizeof(TOKEN_PRIVILEGES);
		if (!LookupPrivilegeValue(NULL, privilege, &luid))
		{
			printf("LookupPrivilegeValue() failed. gle=0x%08x", GetLastError());
			CloseHandle(hToken);
			return false;
		}
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = luid;
		if (enable)
		{
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		}
		else
		{
			tp.Privileges[0].Attributes = 0;
		}

		if (FALSE == AdjustTokenPrivileges(hToken, FALSE, &tp, cb, NULL, NULL))
		{
			DWORD gle = GetLastError();
			if (gle != ERROR_SUCCESS)
			{
				printf("AdjustTokenPrivileges() failed. gle=0x%08x", GetLastError());
				CloseHandle(hToken);
				return false;
			}
		}

		CloseHandle(hToken);
	}

	return true;
}
bool process::kill(_In_ DWORD exit_code)
{
	_ASSERTE(true != _killed);
		if (true == _killed) return true;

	set_privilege(SE_DEBUG_NAME, TRUE);

	HANDLE h = NULL;

	do
	{
		h = OpenProcess(PROCESS_TERMINATE, FALSE, _pid);
		if (NULL == h)
		{
			printf("OpenProcess() failed, pid = %u, gle = %u\n", _pid, GetLastError());
			break;
		}

		if (!TerminateProcess(h, exit_code))
		{
			printf("TerminateProcess() failed, pid = %u, gle = %u\n", _pid, GetLastError());
			break;
		}

		_killed = true;
		printf("pid = %u, %ws terminated\n", _pid, _process_name.c_str());
	} while (false);

	if (NULL != h)
	{
		CloseHandle(h);
	}

	set_privilege(SE_DEBUG_NAME, FALSE);

	return (true == _killed) ? true : false;
}

bool cprocess_tree::build_process_tree()
{
	_proc_map.clear();

	bool ret = false;

	PROCESSENTRY32 proc_entry = { 0 };
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snap == INVALID_HANDLE_VALUE)
	{
		printf("CreateToolhelp32Snapshot() failed, gle = %u\n", GetLastError());
		return false;
	}

	if (true != set_privilege(SE_DEBUG_NAME, TRUE))
	{
		printf("set_privilege(SE_DEBUG_NAME) failed\n");
	}

	do
	{
		proc_entry.dwSize = sizeof(PROCESSENTRY32W);
		if (!Process32First(snap, &proc_entry))
		{
			printf("CreateToolhelp32Snapshot() failed, gle = %u\n", GetLastError());
			break;
		}

		do
		{
			FILETIME create_time = { 0 };
			HANDLE process_handle = OpenProcess(
				PROCESS_QUERY_INFORMATION,
				FALSE,
				proc_entry.th32ProcessID
				);

			if (NULL == process_handle)
			{

			}
			else
			{
				FILETIME dummy_time;
				if (!GetProcessTimes(process_handle, &create_time, &dummy_time, &dummy_time, &dummy_time))
				{
					printf("GetProcessTimes() failed, gle = %u\n", GetLastError());
				}

				CloseHandle(process_handle);
				process_handle = NULL;
			}

			add_process(proc_entry.th32ParentProcessID, proc_entry.th32ProcessID, create_time, proc_entry.szExeFile);
		} while (Process32Next(snap, &proc_entry));
	} while (false);
	CloseHandle(snap);
	set_privilege(SE_DEBUG_NAME, FALSE);

	return true;
}

DWORD cprocess_tree::find_process(_In_ const wchar_t* process_name)
{
	_ASSERT(NULL != process_name);
	if (NULL == process_name) return false;

	process_map::iterator it = _proc_map.begin();
	process_map::iterator ite = _proc_map.end();
	for (; it != ite; ++it)
	{
		if (0 == it->second.process_name().compare(process_name))
		{
			return it->second.pid();
		}
	}

	return 0;
}

const wchar_t* cprocess_tree::get_process_name(_In_ DWORD pid)
{
	process_map::iterator it = _proc_map.find(pid);
	if (it == _proc_map.end()) return NULL;

	return it->second.process_name().c_str();
}

DWORD cprocess_tree::get_parent_pid(_In_ DWORD child_pid)
{
	process_map::iterator it = _proc_map.find(child_pid);
	if (it == _proc_map.end()) return 0;

	process prcs = it->second;

	return prcs.ppid();
}

const wchar_t* cprocess_tree::get_parent_name(_In_ DWORD child_pid)
{
	DWORD ppid = get_parent_pid(child_pid);
	if (0 == ppid) return NULL;

	process_map::iterator it = _proc_map.find(ppid);
	if (it == _proc_map.end()) return NULL;

	return it->second.process_name().c_str();
}

void cprocess_tree::iterate_process(_In_ fnproc_tree_callback callback, _In_ DWORD_PTR callback_tag)
{
	_ASSERT(NULL != callback);
	if (NULL == callback) return;

	process_map::iterator its = _proc_map.begin();
	process_map::iterator ite = _proc_map.end();
	for (; its != ite; ++its)
	{
		if (true != callback(its->second, callback_tag)) break;
	}
}

void cprocess_tree::iterate_process_tree(_In_ DWORD root_pid, _In_ fnproc_tree_callback callback, _In_ DWORD_PTR callback_tag)
{
	_ASSERTE(NULL != callback);
	if (NULL == callback) return;

	process_map::iterator it = _proc_map.find(root_pid);
	if (it == _proc_map.end()) return;
	process root = it->second;

	iterate_process_tree(root, callback, callback_tag);
}

void cprocess_tree::print_process_tree(_In_ DWORD root_pid)
{
	process_map::iterator it = _proc_map.find(root_pid);
	if (it != _proc_map.end())
	{
		DWORD depth = 0;
		print_process_tree(it->second, depth);
		_ASSERTE(0 == depth);
	}
}

void cprocess_tree::print_process_tree(_In_ const wchar_t* root_process_name)
{
	_ASSERTE(NULL != root_process_name);
	if (NULL == root_process_name) { return; }

	DWORD pid = find_process(root_process_name);
	if (0 != pid)
	{
		print_process_tree(pid);
	}
}

bool cprocess_tree::kill_process(_In_ DWORD pid)
{
	if (pid == 0 || pid == 4) return false;

	process_map::iterator it = _proc_map.find(pid);
	if (it == _proc_map.end()) return true;
	process prcs = it->second;

	return prcs.kill(0);
}

bool cprocess_tree::kill_process(_In_ const wchar_t* process_name)
{
	_ASSERTE(NULL != process_name);
	if (NULL == process_name) return false;

	DWORD pid = find_process(process_name);
	return kill_process(pid);
}

bool cprocess_tree::kill_process_tree(_In_ DWORD root_pid)
{
	if (root_pid == 0 || root_pid == 4) return false;

	process_map::iterator it = _proc_map.find(root_pid);
	if (it == _proc_map.end()) return true;
	process root = it->second;

	
	if (true == root.killed())
	{
		printf("already killed. pid = %u, %ws\n", root.pid(), root.process_name().c_str());
			return true;
	}


	kill_process_tree(root);

	return true;
}

void
cprocess_tree::add_process(
	_In_ DWORD ppid,
	_In_ DWORD pid,
	_In_ FILETIME& creation_time,
	_In_ const wchar_t* process_name
	)
{
	process p(process_name, ppid, pid, *(uint64_t*)&creation_time, false);
	_proc_map.insert(std::make_pair(pid, p));
}

void cprocess_tree::print_process_tree(_In_ process& p, _In_ DWORD& depth)
{
	std::wstringstream prefix;
	for (DWORD i = 0; i < depth; ++i)
	{
		prefix << "   ";
	}

	fwprintf(stdout, L"%spid = %u (ppid = %u), %ws \n", prefix.str().c_str(),
		p.pid(),
		p.ppid(),
		p.process_name().c_str()
		);

		// p._pid �� ppid �� ���� item �� ã��
		process_map::iterator it = _proc_map.begin();
	process_map::iterator ite = _proc_map.end();
	for (; it != ite; ++it)
	{
		// ppid �� ���� ���������� ppid ���μ����� �̹� ����ǰ�, ���ο� ���μ����� �����ǰ�, ppid �� �Ҵ���� ��찡 
		// �߻��� �� �ִ�. ���� ppid ���� ������ ��� ppid �� ���� ���μ����� ���� �ð��� pid �� �����ð� ���� �� Ŀ�� �Ѵ�.
		if (it->second.pid() != 0 &&
			it->second.ppid() == p.pid() &&
			(uint64_t)it->second.creation_time() >= (uint64_t)p.creation_time())
		{
			print_process_tree(it->second, ++depth);
			--depth;
		}
	}
}

void cprocess_tree::kill_process_tree(_In_ process& root)
{
	process_map::iterator its = _proc_map.begin();
	process_map::iterator ite = _proc_map.end();
	for (; its != ite; ++its)
	{
		if (its->second.pid() != 0 &&
			its->second.ppid() == root.pid() &&
			its->second.creation_time() >= root.creation_time())
		{
			kill_process_tree(its->second);
		}
	}

	root.kill(0);
}


void cprocess_tree::iterate_process_tree(_In_ process& root, _In_ fnproc_tree_callback callback, _In_ DWORD_PTR callback_tag)
{
	if (true != callback(root, callback_tag)) return;

	process_map::iterator its = _proc_map.begin();
	process_map::iterator ite = _proc_map.end();
	for (; its != ite; ++its)
	{
		if (its->second.pid() != 0 &&
			its->second.ppid() == root.pid() &&
			its->second.creation_time() >= root.creation_time())
		{
			iterate_process_tree(its->second, callback, callback_tag);
		}
	}
}

