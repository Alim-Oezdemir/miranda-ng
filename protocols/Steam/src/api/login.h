#ifndef _STEAM_REQUEST_LOGIN_H_
#define _STEAM_REQUEST_LOGIN_H_

class LogonRequest : public HttpRequest
{
public:
	LogonRequest(const char *token) :
		HttpRequest(HttpPost, STEAM_API_URL "/ISteamWebUserPresenceOAuth/Logon/v0001")
	{
		char data[256];
		mir_snprintf(data, "access_token=%s&ui_mode=web", token);

		Content = new FormUrlEncodedContent(this)
			<< CHAR_PARAM("access_token", token)
			<< CHAR_PARAM("ui_mode", "web");
	}

	//{
	//	"steamid": "XXXXXXXXXXXXXXXXX",
	//	"error" : "OK",
	//	"umqid" : "XXXXXXXXXXXXXXXXXXX",
	//	"timestamp" : 16955891,
	//	"utc_timestamp" : 1514974537,
	//	"message" : 1,
	//	"push" : 0
	//}

};

class LogoffRequest : public HttpRequest
{
public:
	LogoffRequest(const char *token, const char *umqId) :
		HttpRequest(HttpPost, STEAM_API_URL "/ISteamWebUserPresenceOAuth/Logoff/v0001")
	{
		Content = new FormUrlEncodedContent(this)
			<< CHAR_PARAM("access_token", token)
			<< CHAR_PARAM("umqid", umqId);
	}

	//{
	//	"error": "OK"
	//}
};

#endif //_STEAM_REQUEST_LOGIN_H_
