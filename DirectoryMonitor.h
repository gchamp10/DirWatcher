#pragma once
#include <map>
#include <list>
#include <set>
#include <Windows.h>

struct stCompletedDirInfo
{

	std::string filepath;
	std::string dirname;

	stCompletedDirInfo()
	{
		filepath="";
		dirname="";
	}

	stCompletedDirInfo(const stCompletedDirInfo& obj)
	{
		filepath=obj.filepath;
		dirname= obj.dirname;
	}

};
			
struct stSingleFileInfo
{
	DWORD filesize;
	bool bFileCopied;
	stSingleFileInfo()
	{
		filesize =0;
		bFileCopied =0;
	}

	stSingleFileInfo(const stSingleFileInfo& obj)
	{
		filesize =obj.filesize;
		bFileCopied =obj.bFileCopied ;

	}
};

typedef struct _stDirInfo
{

	int nFileCount;
	int nFileCountCompletlyCopied;
	std::map<std::string,stSingleFileInfo> mapListofFileFound;
	_stDirInfo()
	{
		nFileCount=0;
		nFileCountCompletlyCopied=0;
	}
	~_stDirInfo()
	{
		mapListofFileFound.clear();
		nFileCount=0;
		nFileCountCompletlyCopied=0;
	}

}stDirInfo;

class CDirectoryMonitor
{
public:

	static		CDirectoryMonitor* GetInstance(std::string dirname);
	static		void			ReleaseInstance();

	void GetNextPcapfile(std::string &file , std::string &dir);
	HANDLE m_hFileCopiedNotification;

	int RemoveFileinfo(std::string &file , std::string &dir);

	static unsigned int m_dwTotalFileFound;
	static unsigned int m_dwTotalFileProcessed;

private:

	CDirectoryMonitor(void);
	~CDirectoryMonitor(void);

	static CDirectoryMonitor *m_pDirectoryMonitor;

	std::map<std::string,stDirInfo> m_mapFoundFiles;

	

	HANDLE m_hThread;
	HANDLE m_hEvent;

	CRITICAL_SECTION CriticalSection;

	static DWORD WINAPI MyThreadFunction( LPVOID lpParam );

	bool SearchSubFolderinDirectory(LPCTSTR pstrDir);
	bool SearchFilesinSubDirectory(LPCTSTR pstrDir,std::string dirname);

	bool CheckIfFileCopiedCompletely();
	int InitInstance();


	bool m_bStopThread;
	std::string m_dirPath;

	std::list<stCompletedDirInfo> m_listCopiedFile;

	std::string m_curDir;

	
};

