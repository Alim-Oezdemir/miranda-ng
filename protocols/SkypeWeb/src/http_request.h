/*
Copyright (c) 2015-18 Miranda NG team (https://miranda-ng.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

struct VALUE
{
	LPCSTR szName;
	__forceinline VALUE(LPCSTR _name) : szName(_name) { }
};

struct INT_VALUE : public VALUE
{
	int iValue;
	__forceinline INT_VALUE(LPCSTR _name, int _value)
		: VALUE(_name), iValue(_value) { }
};

struct LONG_VALUE : public VALUE
{
	LONGLONG llValue;
	__forceinline LONG_VALUE(LPCSTR _name, LONGLONG _value)
		: VALUE(_name), llValue(_value) { }
};

struct CHAR_VALUE : public VALUE
{
	LPCSTR szValue;
	__forceinline CHAR_VALUE(LPCSTR _name, LPCSTR _value)
		: VALUE(_name), szValue(_value) { }
};

struct FORMAT_VALUE : public VALUE
{
	CMStringA szValue;
	FORMAT_VALUE(LPCSTR _name, LPCSTR _valueFormat, ...)
		: VALUE(_name)
	{
		va_list args;
		va_start(args, _valueFormat);
		szValue.FormatV(_valueFormat, args);
		va_end(args);
	}
};

class HttpRequest : public NETLIBHTTPREQUEST, public MZeroedObject, private MNonCopyable
{
	va_list formatArgs;
	CMStringA url;

protected:
	enum HttpRequestUrlFormat { FORMAT };

	class HttpRequestUrl : private MNonCopyable
	{
		friend HttpRequest;

	private:
		HttpRequest &request;

		HttpRequestUrl(HttpRequest &request, const char *url) : request(request)
		{
			request.url = url;
			request.szUrl = request.url.GetBuffer();
		}

		HttpRequestUrl(HttpRequest &request, const char *urlFormat, va_list args) : request(request)
		{
			request.url.AppendFormatV(urlFormat, args);
			request.szUrl = request.url.GetBuffer();
		}

	public:
		HttpRequestUrl &operator<<(const VALUE &param)
		{
			request.AddUrlParameter(param.szName);
			return *this;
		}

		HttpRequestUrl &operator<<(const INT_VALUE &param)
		{
			request.AddUrlParameter("%s=%i", param.szName, param.iValue);
			return *this;
		}

		HttpRequestUrl &operator<<(const LONG_VALUE &param)
		{
			request.AddUrlParameter("%s=%lld", param.szName, param.llValue);
			return *this;
		}

		HttpRequestUrl &operator<<(const CHAR_VALUE &param)
		{
			request.AddUrlParameter("%s=%s", param.szName, param.szValue);
			return *this;
		}

		char *ToString()
		{
			return request.url.GetBuffer();
		}
	};

	class HttpRequestHeaders : private MNonCopyable
	{
		HttpRequest &request;

		void Add(LPCSTR szName)
		{
			Add(szName, "");
		}

		void Add(LPCSTR szName, LPCSTR szValue)
		{
			request.headers = (NETLIBHTTPHEADER*)mir_realloc(
				request.headers, sizeof(NETLIBHTTPHEADER) * (request.headersCount + 1));
			request.headers[request.headersCount].szName = mir_strdup(szName);
			request.headers[request.headersCount].szValue = mir_strdup(szValue);
			request.headersCount++;
		}

	public:
		HttpRequestHeaders(HttpRequest &request) : request(request) { }

		HttpRequestHeaders& operator<<(const VALUE &param)
		{
			Add(param.szName);
			return *this;
		}

		HttpRequestHeaders& operator<<(const CHAR_VALUE &param)
		{
			Add(param.szName, param.szValue);
			return *this;
		}

		HttpRequestHeaders& operator<<(const FORMAT_VALUE &param)
		{
			Add(param.szName, param.szValue);
			return *this;
		}
	};

	class HttpRequestBody
	{
	private:
		CMStringA content;

		void AppendSeparator()
		{
			if (!content.IsEmpty())
			{
				content.AppendChar('&');
			}
		}

	public:
		HttpRequestBody() { }

		HttpRequestBody & operator<<(const VALUE &param)
		{
			AppendSeparator();
			content.Append(param.szName);
			return *this;
		}

		HttpRequestBody & operator<<(const INT_VALUE &param)
		{
			AppendSeparator();
			content.AppendFormat("%s=%i", param.szName, param.iValue);
			return *this;
		}

		HttpRequestBody & operator<<(const LONG_VALUE &param)
		{
			AppendSeparator();
			content.AppendFormat("%s=%lld", param.szName, param.llValue);
			return *this;
		}

		HttpRequestBody & operator<<(const CHAR_VALUE &param)
		{
			AppendSeparator();
			content.AppendFormat("%s=%s", param.szName, param.szValue);
			return *this;
		}

		HttpRequestBody & operator<<(const FORMAT_VALUE &param)
		{
			AppendSeparator();
			content.AppendFormat("%s=%s", param.szName, param.szValue.c_str());
			return *this;
		}

		char * ToString()
		{
			return content.GetBuffer();
		}
	};

	void AddUrlParameter(const char *fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		url += (url.Find('?') == -1) ? '?' : '&';
		url.AppendFormatV(fmt, args);
		va_end(args);
		szUrl = url.GetBuffer();
	}

public:
	HttpRequestUrl Url;
	HttpRequestHeaders Headers;
	HttpRequestBody Body;

	HttpRequest(int type, LPCSTR url)
		: Url(*this, url), Headers(*this)
	{
		cbSize = sizeof(NETLIBHTTPREQUEST);
		flags = NLHRF_HTTP11 | NLHRF_SSL | NLHRF_DUMPASTEXT;
		requestType = type;
		pData = NULL;
	}

	HttpRequest(int type, HttpRequestUrlFormat, LPCSTR urlFormat, ...)
		: Url(*this, urlFormat, (va_start(formatArgs, urlFormat), formatArgs)), Headers(*this)
	{
		cbSize = sizeof(NETLIBHTTPREQUEST);
		flags = NLHRF_HTTP11 | NLHRF_SSL | NLHRF_DUMPASTEXT;
		requestType = type;
		va_end(formatArgs);
		pData = NULL;
	}

	virtual ~HttpRequest()
	{
		for (int i = 0; i < headersCount; i++)
		{
			mir_free(headers[i].szName);
			mir_free(headers[i].szValue);
		}
		mir_free(headers);
	}

	virtual NETLIBHTTPREQUEST* Send(HNETLIBUSER nlu)
	{
		if (url.Find("://") == -1)
			url.Insert(0, ((flags & NLHRF_SSL) ? "https://" : "http://"));
		szUrl = url.GetBuffer();

		if (!pData) {
			pData = Body.ToString();
			dataLength = (int)mir_strlen(pData);
		}

		Netlib_Logf(nlu, "Send request to %s", szUrl);

		return Netlib_HttpTransaction(nlu, this);
	}
};

#endif //_HTTP_REQUEST_H_