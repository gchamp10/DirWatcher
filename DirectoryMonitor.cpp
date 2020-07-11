#include "StdAfx.h"
#include "DirectoryMonitor.h"
#include "..//Utilities//Helpers.h"



#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")


CDirectoryMonitor *CDirectoryMonitor::m_pDirectoryMonitor = nullptr;
unsigned int CDirectoryMonitor::m_dwTotalFileFound = 0;
unsigned int CDirectoryMonitor::m_dwTotalFileProcessed = 0;;

CDirectoryMonitor::CDirectoryMonitor(void)
{
	InitInstance();
}


CDirectoryMonitor::~CDirectoryMonitor(void)
{
	
	m_bStopThread = true;
	SetEvent(m_hEvent);

	CloseHandle(m_hEvent);
	CloseHandle(m_hFileCopiedNotification);
	CloseHandle(m_hThread);

}

int CDirectoryMonitor::InitInstance()
{
	m_hEvent=CreateEvent(NULL,TRUE,FALSE,NULL);
	m_hFileCopiedNotification=CreateEvent(NULL,TRUE,FALSE,NULL);

	m_bStopThread = false;

	SetEvent(m_hEvent);

	InitializeCriticalSection(&CriticalSection);
	//char buf[_MAX_PATH]={0};
	//u32 dwLen = _MAX_PATH;
	//CMavenConfigHelper *pobjMavenConfigHelper = CMavenConfigHelper::GetInstance();

	//	pobjMavenConfigHelper->ReadString(FPD_CONFIG_FILE_NAME,
	//	"OFFLINE_FILE_PROCESS",
	//	"InputFileDir",
	//	buf,
	//	&dwLen,
	//	"");

	//	m_dirPath = buf;

		        // Create the thread to begin execution on its own.

       m_hThread = CreateThread( 
            NULL,                   // default security attributes
            0,                      // use default stack size  
            MyThreadFunction,       // thread function name
            this,          // argument to thread function 
            0,                      // use default creation flags 
            0);   // returns the thread identifier


	return 0;
}

CDirectoryMonitor* CDirectoryMonitor::GetInstance(std::string dirname)
{
	if(m_pDirectoryMonitor) return m_pDirectoryMonitor;

	m_pDirectoryMonitor = new CDirectoryMonitor();
	m_pDirectoryMonitor->m_dirPath = dirname;

	return m_pDirectoryMonitor;
}

void CDirectoryMonitor::ReleaseInstance()
{
	if(m_pDirectoryMonitor)
	{
		delete m_pDirectoryMonitor;
		m_pDirectoryMonitor = nullptr;
	}

}

void CDirectoryMonitor::GetNextPcapfile(std::string &file,std::string &dir)
{
	if(m_listCopiedFile.empty()) return;

	stCompletedDirInfo l_objDirInfo;
	std::list<stCompletedDirInfo> l_listCopiedFile;

	if(!m_listCopiedFile.empty())
	{
		EnterCriticalSection(&CriticalSection);

		l_listCopiedFile = m_listCopiedFile;

		LeaveCriticalSection(&CriticalSection);
	}


	l_objDirInfo = l_listCopiedFile.front();

	if("" == m_curDir) m_curDir = l_objDirInfo.dirname;

	if(l_objDirInfo.dirname != m_curDir)
	{
		if(m_mapFoundFiles.find(m_curDir)->second.nFileCount != 0)
		{
			std::list<stCompletedDirInfo>::iterator it;
			while(it != l_listCopiedFile.end())
			{
				if(it->dirname == m_curDir)
				{
					file = l_objDirInfo.filepath;
					dir = l_objDirInfo.dirname;

					EnterCriticalSection(&CriticalSection);

					m_listCopiedFile.erase(m_listCopiedFile.begin(),it);;

					LeaveCriticalSection(&CriticalSection);

					return;
				}

				it++;
			}
		}

		m_curDir = l_objDirInfo.dirname;
	}

	EnterCriticalSection(&CriticalSection);

	m_listCopiedFile.erase(m_listCopiedFile.begin());;

	LeaveCriticalSection(&CriticalSection);
	


	file = l_objDirInfo.filepath;
	dir = l_objDirInfo.dirname;
}

int CDirectoryMonitor::RemoveFileinfo(std::string &file , std::string &dir)
{
	stDirInfo *objdirinfo;
	objdirinfo = &(m_mapFoundFiles.find(dir)->second);

	if(objdirinfo->mapListofFileFound.find(file) != objdirinfo->mapListofFileFound.end())
	objdirinfo->mapListofFileFound.erase(objdirinfo->mapListofFileFound.find(file));

	objdirinfo->nFileCount--;
	objdirinfo->nFileCountCompletlyCopied--;

	return 0;
}

DWORD WINAPI CDirectoryMonitor::MyThreadFunction( LPVOID lpParam )
{
	CDirectoryMonitor *l_pDirectoryMonitor = (CDirectoryMonitor *)lpParam;
	while(!l_pDirectoryMonitor->m_bStopThread)
	{
		WaitForSingleObject(l_pDirectoryMonitor->m_hEvent,5000);

		if(l_pDirectoryMonitor->m_bStopThread) 
			break;

		ResetEvent(l_pDirectoryMonitor->m_hEvent);
		 
		l_pDirectoryMonitor->SearchSubFolderinDirectory(LPCTSTR(l_pDirectoryMonitor->m_dirPath.c_str()));

		l_pDirectoryMonitor->CheckIfFileCopiedCompletely();
	}

	return 0;
}

bool CDirectoryMonitor::SearchSubFolderinDirectory(LPCTSTR pstrDir)
{
	WIN32_FIND_DATA wfd;
	HANDLE			hFind;
	TCHAR			szPath[MAX_PATH];
	bool			bRecurse = true;
	_tcsncpy(szPath, pstrDir, MAX_PATH);
	//////////////////////////////////

	::PathAppend(szPath, _T("*"));

	// Iterate through dirs

	hFind = ::FindFirstFile(szPath, &wfd);
	if(hFind != INVALID_HANDLE_VALUE)
	{
		do
		{

			// FIRST check if its a dir

			if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{

				// if not a DOT, it will not even do the string compare

				if(!((wfd.cFileName[0] == '.') && ((wfd.cFileName[1] == '.') || (wfd.cFileName[1] == '\0'))))
				{
					if(bRecurse)
					{
						// Recurse Dirs

						TCHAR szNextPath[MAX_PATH];
						::PathCombine(szNextPath, pstrDir, wfd.cFileName);

						std::string strdirname(wfd.cFileName);

						if(m_mapFoundFiles.find(strdirname) == m_mapFoundFiles.end())
						{
							stDirInfo objdirinfo;

							//std::string strdirname(dirname.begin(),dirname.end());
							m_mapFoundFiles[strdirname] = objdirinfo;
						}

						SearchFilesinSubDirectory(szNextPath,strdirname);
					}
				}
			}
		} while(::FindNextFile(hFind, &wfd));
		::FindClose(hFind);
	}

	return true;
}

bool CDirectoryMonitor::SearchFilesinSubDirectory(LPCTSTR pstrDir,std::string dirname)
{
	WIN32_FIND_DATA wfd;
	HANDLE			hFind;
	TCHAR			szPath[MAX_PATH];
	bool			bRecurse = true;
	_tcsncpy(szPath, pstrDir, MAX_PATH);
	//////////////////////////////////

	::PathAppend(szPath, _T("*.pcap"));

	hFind = ::FindFirstFile(szPath, &wfd);
	if(hFind != INVALID_HANDLE_VALUE)
	{
		do
		{

			if((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				
					TCHAR szNextFile[MAX_PATH];
					_tcsncpy(szNextFile, pstrDir, MAX_PATH);
					::PathAppend(szNextFile, wfd.cFileName);

					if(m_mapFoundFiles.find(dirname)!=m_mapFoundFiles.end())
					{
						stDirInfo *objdirinfo;
						objdirinfo = &(m_mapFoundFiles.find(dirname)->second);
						

						std::string strfilename( szNextFile);
						//std::string strfilename(filename.begin(),filename.end());
						if(objdirinfo->mapListofFileFound.find(strfilename)==objdirinfo->mapListofFileFound.end())
						{
							objdirinfo->nFileCount++;
							stSingleFileInfo objfileinfo;
							objfileinfo.filesize = wfd.nFileSizeLow;

							objdirinfo->mapListofFileFound[strfilename] = objfileinfo;
						}
					}

			}
		} while((::FindNextFile(hFind, &wfd)));
		::FindClose(hFind);
	}
	return true;
}



bool CDirectoryMonitor::CheckIfFileCopiedCompletely()
{
	std::map<std::string,stDirInfo>::iterator it=m_mapFoundFiles.begin();
	while(it != m_mapFoundFiles.end())
	{
		//if all the file are already copied in a dir then dont go for it
		if(it->second.nFileCount == it->second.nFileCountCompletlyCopied)
			it++;

		std::map<std::string,stSingleFileInfo> *l_mapListofFileFound;
		l_mapListofFileFound = &((*it).second.mapListofFileFound);
		std::map<std::string,stSingleFileInfo>::iterator fileitr = l_mapListofFileFound->begin();

		while( fileitr != l_mapListofFileFound->end() )
		{
			if(false == fileitr->second.bFileCopied)
			{
				LONGLONG currentsize=0;
				char* filename = const_cast<char*>(fileitr->first.c_str());

				ComputeFileSize(filename,currentsize);

				if(0 == currentsize - fileitr->second.filesize)
				{
					
					stCompletedDirInfo l_objDirInfo;
					l_objDirInfo.filepath = fileitr->first;
					l_objDirInfo.dirname = it->first;
				
				// add to the list of copied file
					EnterCriticalSection(&CriticalSection);
					m_listCopiedFile.push_back(l_objDirInfo);
					LeaveCriticalSection(&CriticalSection);

					SetEvent(m_hFileCopiedNotification);

					(*fileitr).second.bFileCopied = true;
					it->second.nFileCountCompletlyCopied++;

					m_dwTotalFileFound++;
				}

				(*fileitr).second.filesize = currentsize;
			}
			*fileitr++;
		}
		*it++;
	}

	return true;
}

