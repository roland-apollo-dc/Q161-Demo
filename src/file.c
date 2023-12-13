#include <string.h>

#include "def.h"
#include <coredef.h>
#include <struct.h>
#include <poslib.h>

#define ZIPPATH "/ext/"


u8* filename(u8* file)
{
	return file;
}
void filegetlistcbtesting(const char *pchDirName, uint32 size, uint8 filetype, void *arg)
{
	char *pChar;
	int ret,num;

//	MAINLOG_L1("[%s] -%s- Line=%d: size = %d,filetype=%d,pchDirName:%s, arg=%s\r\n", filename(__FILE__), __FUNCTION__, __LINE__,size, filetype, pchDirName, arg);

	//delete all mp3 files
	if (fileFilter==1) {
		pChar = strstr(pchDirName,".mp3");
		if (pChar!=NULL) {
			DelFile_Api(pchDirName);
		}
	}

	//delete all .img files
	if (fileFilter==2) {
		pChar = strstr(pchDirName,".img");
		if (pChar!=NULL) {
			DelFile_Api(pchDirName);
		}
	}

	//search app to update
	if (fileFilter==3) {
		pChar = strstr(pchDirName,".img");
		if (pChar!=NULL) {
			num = 9;
			memset(updateAppName,0,sizeof(updateAppName));
			memcpy(updateAppName,pchDirName,strlen(pchDirName));
			MAINLOG_L1("pchDirName = %s",pchDirName);
			needUpdate = 1;
		}
	}

}
void folderFileDisplay(unsigned char *filePath)
{
	int iRet = -1;

	uint8 *rP = NULL;

	iRet = fileGetFileListCB_lib(filePath, filegetlistcbtesting, rP);

}

int unzipDownFile(unsigned char *fileName){
	int ret;

	ret = fileunZip_lib(fileName, ZIPPATH);
	if(ret != 0){
		return -1;
	}
	DelFile_Api(fileName);//delete zip after unzip it

	return 0;
}

void CheckAppFile(){
	int ret;
	unsigned char buf[32];

	ret = GetFileSize_Api(SAVE_UPDATE_FILE_NAME);
	MAINLOG_L1("GetFileSize_Api = %d", ret);
	if (ret>0) {
		memset(buf,0,sizeof(buf));
		ReadFile_Api(SAVE_UPDATE_FILE_NAME, buf, 0, (unsigned int *)&ret);
		MAINLOG_L1("buf = %s", buf);
		DelFile_Api(buf);
		DelFile_Api(SAVE_UPDATE_FILE_NAME);
	}
}




