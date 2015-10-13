// ProcessViewer.cpp : �ܼ� ���� ���α׷��� ���� �������� �����մϴ�.
//

#include "stdafx.h"


bool proc_tree_callback(_In_ process& process_info, _In_ DWORD_PTR callback_tag)
{
	printf("pid = %u, %u, %ws %llu\n", process_info.pid(), process_info.ppid(), process_info.process_name().c_str(), process_info.creation_time());
		return true;
}
//I'M BBAKBBAK
int main()
{
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 ppe;
	ppe.dwSize = sizeof(PROCESSENTRY32);
	DWORD suspend_pid;
	BOOL b = Process32First(hSnap, &ppe);
	char pname[1024] = { 0 };
	size_t convertedChars = 0;
	int process_cnt = 0;
	LPTSTR szNotepad = _tcsdup(TEXT("winword.exe"));
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	while (b)
	{
		//printf_s("process Name:%S\n", ppe.szExeFile);
		//printf_s("process ID:%6d\n", ppe.th32ProcessID);
		b = Process32Next(hSnap, &ppe);
		wcstombs_s(&convertedChars, pname, sizeof(ppe.szExeFile), ppe.szExeFile, _TRUNCATE);
		if (strcmp(pname, "PING.EXE") == 0)
		{
			process_cnt++;
			suspend_pid = ppe.th32ProcessID;
		}
	}
	CloseHandle(hSnap);

	cprocess_tree proc_tree;
	process proc;
	if (!proc_tree.build_process_tree()) return false;

	// ���μ��� ���� �׽�Ʈ (by callback)
	proc_tree.iterate_process(proc_tree_callback, 0);
	//proc_tree.iterate_process_tree(proc_tree.find_process(L"System"), proc_tree_callback, 0);
	getchar();

	// print 
	proc_tree.print_process_tree(proc_tree.find_process(L"wininit.exe"));
	getchar();

	ShellExecute(NULL, _tcsdup(TEXT("open")), szNotepad, NULL, NULL, SW_SHOW);

	getchar();
	
	proc_tree.build_process_tree();
	// ���μ��� ���� �׽�Ʈ	
	proc_tree.kill_process_tree(proc_tree.find_process(L"winword.exe"));
	
	getchar();
	
	//���μ��� �Ͻ����� �׽�Ʈ
	if (process_cnt > 0)
	{
		printf("suspending ping.exe...\n\n");
		if (!proc.suspend(suspend_pid))
		{
			printf("suspend fail!\n");
			return -1;
		}
		printf("success!! wait...\n");
		getchar();
		//Sleep(5000);
		//���μ��� �Ͻ������� �ٽ� ����
		printf("resuming...\n");
		if (!proc.resume(suspend_pid))
		{
			printf("resume fail!\n");
			return -1;
		}
		printf("success!!!\n");
	}
	else
	{
		printf("process�� �������� �ʽ��ϴ�!\n");
	}

	getchar();

	return 0;
}

 