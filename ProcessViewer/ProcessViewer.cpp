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
	HANDLE hSnap;
	PROCESSENTRY32 ppe;
	ppe.dwSize = sizeof(PROCESSENTRY32);
	DWORD suspend_pid;
	
	char pname[1024] = { 0 };
	size_t convertedChars = 0;
	int process_cnt = 0;
	BOOL b;
	cprocess_tree proc_tree;
	process proc;

	int num, choice, cnt=0;
	while (1)
	{
		printf_s("\n1-list    2-list process tree    3-kill    4-suspend    5-resume\ninput : ");
		scanf_s("%d", &num);
		if (num == 1)
		{
			hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			b = Process32First(hSnap, &ppe);
			while (b)//�ʹݿ� ���� ������ ����
			{
				printf_s("process Name:%S", ppe.szExeFile);
				printf_s("    PID:%6d\n", ppe.th32ProcessID);
				b = Process32Next(hSnap, &ppe);
				cnt++;
				wcstombs_s(&convertedChars, pname, sizeof(ppe.szExeFile), ppe.szExeFile, _TRUNCATE);
				/*if (strcmp(pname, "PING.EXE") == 0)
				{
					process_cnt++;
					suspend_pid = ppe.th32ProcessID;
				}*/
			}
			printf("The number of Prccesses : %d\n", cnt);
			cnt = 0;
		}
		
		else if (num == 2)//process 
		{
			while (b)
			{
				printf_s("�����ؾ���\n");
			}
		}
		else if (num == 3)//kill
		{
			printf_s("kill process input : ");
			scanf_s("%d",&choice);
			//proc.kill((DWORD)choice);
			proc_tree.kill_process_tree((DWORD)choice);
			printf_s("killed ok\n");
		}
		else if (num == 4)//suspend
		{
			printf_s("suspend process input : ");
			scanf_s("%d", &choice);
			proc.suspend((DWORD)choice);
			printf_s("suspend ok\n");
		}
		else if (num == 5)//resume
		{
			printf_s("resume process input : ");
			scanf_s("%d", &choice);
			proc.resume((DWORD)choice);
			printf_s("resume ok\n");
		}
	}

	CloseHandle(hSnap);

	
	if (!proc_tree.build_process_tree()) return false;

	// ���μ��� ���� �׽�Ʈ (by callback)
	//proc_tree.iterate_process(proc_tree_callback, 0);
	//proc_tree.iterate_process_tree(proc_tree.find_process(L"System"), proc_tree_callback, 0);

	// print 
	proc_tree.print_process_tree((DWORD)596);

	// ���μ��� ���� �׽�Ʈ	
	//proc_tree.kill_process_tree(proc_tree.find_process(L"cmd.exe"));

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
		Sleep(5000);
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
	return true;
	
}

