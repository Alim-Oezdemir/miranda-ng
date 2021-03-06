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

#include "stdafx.h"

void CSkypeProto::OnCreateTrouter(const NETLIBHTTPREQUEST *response)
{
	if (response == nullptr || response->pData == nullptr) {
LBL_Error:
		debugLogA("Failed to establish a TRouter connection.");
		return;
	}

	JSONNode root = JSONNode::parse(response->pData);
	if (!root)
		goto LBL_Error;

	const JSONNode &ccid = root["ccid"];
	const JSONNode &connId = root["connId"];
	const JSONNode &instance = root["instance"];
	const JSONNode &socketio = root["socketio"];
	const JSONNode &url = root["url"];

	if (!ccid || !connId || !instance || !socketio || !url)
		goto LBL_Error;

	TRouter.ccid = ccid.as_string();
	TRouter.connId = connId.as_string();
	TRouter.instance = instance.as_string();
	TRouter.socketIo = socketio.as_string();
	TRouter.url = url.as_string();

	SendRequest(new CreateTrouterPoliciesRequest(li, TRouter.connId.c_str()), &CSkypeProto::OnTrouterPoliciesCreated);
}

void CSkypeProto::OnTrouterPoliciesCreated(const NETLIBHTTPREQUEST *response)
{
	if (response == nullptr || response->pData == nullptr) {
LBL_Error:
		debugLogA("Failed to establish a TRouter connection.");
		return;
	}

	JSONNode root = JSONNode::parse(response->pData);
	if (!root)
		goto LBL_Error;

	const JSONNode &st = root["st"];
	const JSONNode &se = root["se"];
	const JSONNode &sig = root["sig"];

	if (!st || !se || !sig)
		goto LBL_Error;

	TRouter.st = st.as_string();
	TRouter.se = se.as_string();
	TRouter.sig = sig.as_string();

	SendRequest(new GetTrouterRequest(
		TRouter.socketIo,
		TRouter.connId,
		TRouter.st,
		TRouter.se,
		TRouter.sig,
		TRouter.instance,
		TRouter.ccid), &CSkypeProto::OnGetTrouter);
}

void CSkypeProto::OnGetTrouter(const NETLIBHTTPREQUEST *response)
{
	if (response == nullptr || response->pData == nullptr) {
		debugLogA("Failed to establish a TRouter connection.");
		return;
	}

	CMStringA data(response->pData);
	int iStart = 0;
	CMStringA szToken = data.Tokenize(":", iStart).Trim();
	TRouter.sessId = szToken.GetString();

	m_hTrouterEvent.Set();
	m_hTrouterHealthEvent.Set();

	if ((time(nullptr) - TRouter.lastRegistrationTime) >= 3600) {
		SendRequest(new RegisterTrouterRequest(li, TRouter.url.c_str(), TRouter.sessId.c_str()));
		TRouter.lastRegistrationTime = time(nullptr);
	}
}

void CSkypeProto::OnHealth(const NETLIBHTTPREQUEST*)
{
	SendRequest(new GetTrouterRequest(
		TRouter.socketIo,
		TRouter.connId,
		TRouter.st,
		TRouter.se,
		TRouter.sig,
		TRouter.instance,
		TRouter.ccid),
		&CSkypeProto::OnGetTrouter);
}

void CSkypeProto::TRouterThread(void*)
{
	debugLogA(__FUNCTION__": entering");

	while (true) {
		m_hTrouterEvent.Wait();
		if (m_bThreadsTerminated)
			break;

		int errors = 0;

		TrouterPollRequest *request = new TrouterPollRequest(TRouter.socketIo, TRouter.connId, TRouter.st, TRouter.se, TRouter.sig, TRouter.instance, TRouter.ccid, TRouter.sessId);

		while (errors < POLLING_ERRORS_LIMIT && m_iStatus > ID_STATUS_OFFLINE) {
			request->nlc = m_TrouterConnection;
			NLHR_PTR response(request->Send(m_hNetlibUser));

			if (response == NULL) {
				m_TrouterConnection = nullptr;
				errors++;
				continue;
			}

			if (response->resultCode == 200) {
				errors = 0;

				if (response->pData) {
					char *json = strstr(response->pData, "{");
					if (json != NULL) {
						JSONNode root = JSONNode::parse(json);
						std::string szBody = root["body"].as_string();
						const JSONNode &headers = root["headers"];
						const JSONNode body = JSONNode::parse(szBody.c_str());
						OnTrouterEvent(body, headers);
					}
				}
			}
			else {
				errors++;

				SendRequest(new HealthTrouterRequest(TRouter.ccid.c_str()), &CSkypeProto::OnHealth);
				m_hTrouterHealthEvent.Wait();
			}
			m_TrouterConnection = response->nlc;
		}
		delete request;

		if (m_iStatus != ID_STATUS_OFFLINE) {
			debugLogA(__FUNCTION__ ": unexpected termination; switching protocol to offline");
			SetStatus(ID_STATUS_OFFLINE);
		}
	}
	m_hTrouterThread = nullptr;
	m_TrouterConnection = nullptr;
	debugLogA(__FUNCTION__": leaving");
}

void CSkypeProto::OnTrouterEvent(const JSONNode &body, const JSONNode &)
{
	//std::string displayname = body["displayName"].as_string();
	//std::string cuid = body["callerId"].as_string();
	std::string uid = body["conversationId"].as_string();
	std::string gp = body["gp"].as_string();
	int evt = body["evt"].as_int();

	switch (evt) {
	case 100: //incoming call
		{
			std::string callId = body["convoCallId"].as_string();
			if (!uid.empty()) {
				MCONTACT hContact = AddContact(uid.c_str(), true);

				MEVENT hEvent = AddDbEvent(SKYPE_DB_EVENT_TYPE_INCOMING_CALL, hContact, time(nullptr), DBEF_READ, gp.c_str(), callId.c_str());
				Skin_PlaySound("skype_inc_call");

				CLISTEVENT cle = {};
				cle.flags = CLEF_UNICODE;
				cle.hContact = hContact;
				cle.hDbEvent = hEvent;
				cle.lParam = SKYPE_DB_EVENT_TYPE_INCOMING_CALL;
				cle.hIcon = GetIcon(IDI_CALL);

				CMStringA service(FORMAT, "%s/IncomingCallCLE", GetContactProto(hContact));
				cle.pszService = service.GetBuffer();

				CMStringW tooltip(FORMAT, TranslateT("Incoming call from %s"), Clist_GetContactDisplayName(hContact));
				cle.szTooltip.w = tooltip.GetBuffer();
				pcli->pfnAddEvent(&cle);

				ShowNotification(Clist_GetContactDisplayName(hContact), TranslateT("Incoming call"), hContact, SKYPE_DB_EVENT_TYPE_INCOMING_CALL);
			}
		}
		break;

	case 104: //call canceled: callerId=""; conversationId=NULL; callId=call id
		// std::string callId = body["callId"].as_string();
		Skin_PlaySound("skype_call_canceled");
		break;
	}
}

INT_PTR CSkypeProto::OnIncomingCallCLE(WPARAM, LPARAM lParam)
{
	CLISTEVENT *cle = (CLISTEVENT*)lParam;
	NotifyEventHooks(g_hCallEvent, (WPARAM)cle->hContact, (LPARAM)0);
	return 0;
}

INT_PTR CSkypeProto::OnIncomingCallPP(WPARAM wParam, LPARAM hContact)
{
	while (CLISTEVENT *cle = pcli->pfnGetEvent(hContact, 0)) {
		if (cle->lParam == SKYPE_DB_EVENT_TYPE_INCOMING_CALL) {
			pcli->pfnRemoveEvent(hContact, cle->hDbEvent);
			break;
		}
	}

	if (wParam == 1)
		NotifyEventHooks(g_hCallEvent, (WPARAM)hContact, (LPARAM)0);

	return 0;
}
