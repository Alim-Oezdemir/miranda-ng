#ifndef M_CLOUDFILE_H_
#define M_CLOUDFILE_H_

struct CFSERVICEINFO
{
	const char *accountName;
	const wchar_t *userName;
};

// get cloud file service info by account name
// wParam = (WPARAM)(const char*)accountName (can be NULL)
// lParam = (LPARAM)(CFSERVICEINFO*)serviceInfo (can be NULL)
// returns 0 on success, nonzero on failure
#define MS_CLOUDFILE_GETSERVICE "CloudFile/GetService"

// return nonzero to stop enum
typedef int(*enumCFServiceFunc)(const CFSERVICEINFO *serviceInfo, void *param);

// get list of cloud file services
// wParam = (WPARAM)(enumCFServiceFunc)enumFunc
// lParam = (LPARAM)(void*)param (can be NULL)
// returns 0 on success, nonzero on failure
#define MS_CLOUDFILE_ENUMSERVICES "CloudFile/EnumServices"

struct CFUPLOADDATA
{
	const char *accountName; // cloud service to upload (can be NULL)
	const wchar_t *localPath; // local path
	const wchar_t *serverFolder; // server folder in witch file will be placed (can be NULL)
};

struct CFUPLOADRESULT
{
	char **links; // need to be freed manually or by calling cfur_free
	size_t linkCount;
	wchar_t *description; // need to be freed manually or by calling cfur_free
};

// frees allocated fields of CFUPLOADRESULT
__inline void cfur_free(CFUPLOADRESULT *ur)
{
	for (size_t i = 0; i < ur->linkCount; i++)
		mir_free(ur->links[0]);
	mir_free(ur->links);
	mir_free(ur->description);
}

// upload file on cloud service
// wParam = (WPARAM)(const CFUPLOADDATA*)uploadData
// lParam = (LPARAM)(char CFUPLOADRESULT*)uploadResult (can be NULL)
// returns 0 on success, nonzero on failure
#define MS_CLOUDFILE_UPLOAD "CloudFile/Upload"

#endif //M_CLOUDFILE_H_