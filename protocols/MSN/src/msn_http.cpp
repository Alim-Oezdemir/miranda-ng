/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.

Copyright (c) 2012-2018 Miranda NG team
Copyright (c) 2006-2012 Boris Krasnovskiy.
Copyright (c) 2003-2005 George Hazan.
Copyright (c) 2002-2003 Richard Hughes (original version).

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"
#include "msn_proto.h"

static ThreadData* FindThreadConn(HNETLIBCONN hConn)
{
	for (auto &it : CMPlugin::g_arInstances) {
		ThreadData *res = it->MSN_GetThreadByConnection(hConn);
		if (res != nullptr)
			return res;
	}

	return nullptr;
}

//=======================================================================================
// Fake function - it does nothing but confirms successful session initialization
//=======================================================================================

int msn_httpGatewayInit(HNETLIBCONN hConn, NETLIBOPENCONNECTION*, NETLIBHTTPREQUEST*)
{
	NETLIBHTTPPROXYINFO nlhpi = {};
	nlhpi.szHttpGetUrl = nullptr;
	nlhpi.szHttpPostUrl = "messenger.hotmail.com";
	nlhpi.flags = NLHPIF_HTTP11;
	nlhpi.combinePackets = MSN_PACKETS_COMBINE;
	return Netlib_SetHttpProxyInfo(hConn, &nlhpi);
}

//=======================================================================================
// Prepares the szHttpPostUrl. If it's the very first send (mSessionID is void), this
// function generates the initial URL depending on a thread type
//=======================================================================================

int msn_httpGatewayWrapSend(HNETLIBCONN hConn, PBYTE buf, int len, int flags)
{
	ThreadData *T = FindThreadConn(hConn);
	if (T != nullptr) {
		if (T->sessionClosed)
			return SOCKET_ERROR;

		T->applyGatewayData(hConn, len == 0);
	}

	return Netlib_Send(hConn, (char*)buf, len, flags);
}

//=======================================================================================
// Processes the results of the command execution. Parses HTTP headers to get the next
// SessionID & gateway IP values
//=======================================================================================

PBYTE msn_httpGatewayUnwrapRecv(NETLIBHTTPREQUEST* nlhr, PBYTE buf, int len, int *outBufLen, void *(*)(void *, size_t))
{
	*outBufLen = len;

	ThreadData *T = FindThreadConn(nlhr->nlc);
	if (T == nullptr)
		return buf;

	bool isSessionClosed = true;
	bool isMsnPacket = false;

	if (nlhr->resultCode == 200) {
		char *xMsgr = nullptr, *xHost = nullptr;

		for (int i = 0; i < nlhr->headersCount; i++) {
			NETLIBHTTPHEADER& tHeader = nlhr->headers[i];
			if (_stricmp(tHeader.szName, "X-MSN-Messenger") == 0)
				xMsgr = tHeader.szValue;
			else if (_stricmp(tHeader.szName, "X-MSN-Host") == 0)
				xHost = tHeader.szValue;

		}

		if (xMsgr) {
			isMsnPacket = true;

			if (strstr(xMsgr, "Session=close") == nullptr)
				isSessionClosed = false;

			T->processSessionData(xMsgr, xHost);
			T->applyGatewayData(nlhr->nlc, false);
		}
	}

	T->sessionClosed |= isSessionClosed;
	if (isSessionClosed && buf == nullptr) {
		*outBufLen = 0;
		buf = (PBYTE)mir_alloc(1);
		*buf = 0;
	}
	else if (buf == nullptr && len == 0) {
		*outBufLen = 1;
		buf = (PBYTE)mir_alloc(1);
		*buf = 0;
	}
	else if (!isMsnPacket) {
		*outBufLen = 0;
		*buf = 0;
	}
	return buf;
}
