// DirectoryMonitoringUnit.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "DirectoryMonitor.h"
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <iostream>
#include <string>


int _tmain(int argc, _TCHAR* argv[])
{
	char filepath[1024];
	std::cout << "enter the directory path\n";
	std::cin >> filepath;
	CDirectoryMonitor *m_ptr = CDirectoryMonitor::GetInstance(filepath);

	std::string file;
	std::string dir;

	WaitForSingleObject(m_ptr->m_hFileCopiedNotification,10000);
	ResetEvent(m_ptr->m_hFileCopiedNotification);
	m_ptr->GetNextPcapfile(file,dir);

	while(!file.empty())
	{
		std::cout<<"SubDir:"<<dir.c_str()<<"\t"<<"Filename:"<<file.c_str()<<std::endl;

		m_ptr->RemoveFileinfo(file,dir);

		std::string newname = file + ".done";
		MoveFile(file.c_str(),newname.c_str());
		WaitForSingleObject(m_ptr->m_hFileCopiedNotification,5000);

		ResetEvent(m_ptr->m_hFileCopiedNotification);
		m_ptr->GetNextPcapfile(file,dir);
	}

	return 0;
}

