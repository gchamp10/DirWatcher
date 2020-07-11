// DirectoryMonitoringUnit.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "DirectoryMonitor.h"

int _tmain(int argc, _TCHAR* argv[])
{
	CDirectoryMonitor *m_ptr = CDirectoryMonitor::GetInstance("D:\\");

	std::string file;
	std::string dir;
	m_ptr->GetNextPcapfile(file,dir);


	return 0;
}

