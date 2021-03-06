#include "stdafx.h"

void CSteamProto::ParsePollData(const JSONNode &data)
{
	std::string steamIds;

	for (const JSONNode &item : data) {
		json_string steamId = item["steamid_from"].as_string();
		time_t timestamp = _wtol(item["utc_timestamp"].as_mstring());

		MCONTACT hContact = NULL;
		if (!IsMe(steamId.c_str()) && !(hContact = GetContact(steamId.c_str())))
			// probably this is info about random player playing on same server, so we ignore it
			continue;

		json_string type = item["type"].as_string();
		if (type == "my_saytext" || type =="my_emote") {
			json_string text = item["text"].as_string();

			PROTORECVEVENT recv = { 0 };
			recv.timestamp = timestamp;
			recv.szMessage = (char*)text.c_str();
			recv.flags = PREF_SENT;
			RecvMsg(hContact, &recv);

			continue;
		}

		if (type == "saytext" || type =="emote") {
			json_string text = item["text"].as_string();

			PROTORECVEVENT recv = { 0 };
			recv.timestamp = timestamp;
			recv.szMessage = (char*)text.c_str();
			ProtoChainRecvMsg(hContact, &recv);

			CallService(MS_PROTO_CONTACTISTYPING, hContact, (LPARAM)PROTOTYPE_CONTACTTYPING_OFF);
			m_typingTimestamps[steamId] = 0;

			continue;
		}

		if (type == "typing") {
			auto it = m_typingTimestamps.find(steamId);
			if (it != m_typingTimestamps.end()) {
				if ((timestamp - it->second) < STEAM_TYPING_TIME)
					continue;
			}
			CallService(MS_PROTO_CONTACTISTYPING, hContact, (LPARAM)STEAM_TYPING_TIME);
			m_typingTimestamps[steamId] = timestamp;

			continue;
		}

		if (type == "personastate") {
			if (!IsMe(steamId.c_str())) {
				// there no sense to change own status
				JSONNode node = item["persona_state"];
				if (!node.isnull()) {
					int status = SteamToMirandaStatus((PersonaState)node.as_int());
					SetContactStatus(hContact, status);
				}
			}
			steamIds.append(steamId).append(",");

			continue;
		}

		if (type == "personarelationship") {
			PersonaRelationshipAction state = (PersonaRelationshipAction)item["persona_state"].as_int();
			switch (state) {
			case PersonaRelationshipAction::Remove:
				hContact = GetContact(steamId.c_str());
				if (hContact)
					ContactIsRemoved(hContact);
				break;

			case PersonaRelationshipAction::Ignore:
				hContact = GetContact(steamId.c_str());
				if (hContact)
					ContactIsBlocked(hContact);
				break;

			case PersonaRelationshipAction::AuthRequest:
				hContact = AddContact(steamId.c_str());
				if (hContact)
					ContactIsAskingAuth(hContact);
				else {
					// load info about this user from server
					ptrA token(getStringA("TokenSecret"));
					PushRequest(
						new GetUserSummariesRequest(token, steamId.c_str()),
						&CSteamProto::OnAuthRequested);
				}
				break;

			case PersonaRelationshipAction::AuthRequested:
				hContact = GetContact(steamId.c_str());
				if (hContact)
					ContactIsFriend(hContact);
				break;
			}

			continue;
		}
		
		if (type == "leftconversation") {
			if (!getBool("ShowChatEvents", true))
				continue;

			BYTE bEventType = STEAM_DB_EVENT_CHATSTATES_GONE;
			DBEVENTINFO dbei = {};
			dbei.pBlob = &bEventType;
			dbei.cbBlob = 1;
			dbei.eventType = EVENTTYPE_STEAM_CHATSTATES;
			dbei.flags = DBEF_READ;
			dbei.timestamp = now();
			dbei.szModule = m_szModuleName;
			db_event_add(hContact, &dbei);

			continue;
		}

		debugLogA(__FUNCTION__ ": Unknown event type \"%s\"", type.c_str());
	}

	if (steamIds.empty())
		return;

	steamIds.pop_back();
	ptrA token(getStringA("TokenSecret"));
	PushRequest(
		new GetUserSummariesRequest(token, steamIds.c_str()),
		&CSteamProto::OnGotUserSummaries);
}

struct PollParam
{
	int errors;
	int errorsLimit;
};

void CSteamProto::OnGotPoll(const HttpResponse &response, void *arg)
{
	PollParam *param = (PollParam*)arg;
	if (!response) {
		// bad response
		debugLogA(__FUNCTION__ ": server returns bad response (%d)", response.GetStatusCode());
		param->errors++;
		Sleep(STEAM_API_TIMEOUT * 1000);
		return;
	}

	// handling of known errors
	if (!response.IsSuccess()) {
		switch (response.GetStatusCode()) {
		case HTTP_CODE_SERVICE_UNAVAILABLE:
			// server on maintenance
			SetAllContactStatuses(ID_STATUS_OFFLINE);
			Sleep(STEAM_API_TIMEOUT * 1000);
			return;

		case HTTP_CODE_UNAUTHORIZED:
			// token has expired
			debugLogA(__FUNCTION__ ": access is denied");
			delSetting("TokenSecret");
			param->errors = param->errorsLimit;
			return;

		default:
			debugLogA(__FUNCTION__ ": server returns unexpected status code (%d)", response.GetStatusCode());
			param->errors++;
			Sleep(STEAM_API_TIMEOUT * 1000);
			return;
		}
	}

	JSONNode root = JSONNode::parse(response.Content);
	if (root.isnull()) {
		debugLogA(__FUNCTION__ ": could not recognize a response");
		param->errors++;
		return;
	}

	json_string error = root["error"].as_string();
	if (error == "Timeout")
		// do nothing as this is not necessarily an error
		return;

	if (error == "OK") {
		// remember last message timestamp
		time_t timestamp = _wtoi64(root["utc_timestamp"].as_mstring());
		if (timestamp > getDword("LastMessageTS", 0))
			setDword("LastMessageTS", timestamp);

		long messageId = root["messagelast"].as_int();
		setDword("MessageID", messageId);

		JSONNode data = root["messages"].as_array();
		ParsePollData(data);

		// Reset error counter only when we've got OK
		param->errors = 0;

		// m_pollingConnection = response->nlc;

		return;
	}

	if (error == "Not Logged On") {
		// need to relogin
		debugLogA(__FUNCTION__ ": not logged on");
		param->errors = param->errorsLimit;
		// try to reconnect only when we're actually online (during normal logout we will still got this error anyway, but in that case our status is already offline)
		if (IsOnline()) {
			ptrA token(getStringA("TokenSecret"));
			SendRequest(
				new LogonRequest(token),
				&CSteamProto::OnReLogin);
		}
		return;
	}

	// something wrong
	debugLogA(__FUNCTION__ ": %s (%d)", error.c_str(), response.GetStatusCode());

	// too low timeout?
	int timeout = root["sectimeout"].as_int();
	if (timeout < STEAM_API_TIMEOUT)
		debugLogA(__FUNCTION__ ": Timeout is too low (%d)", timeout);

	// let it jump out of further processing
	param->errors = param->errorsLimit;
}

void CSteamProto::PollingThread(void*)
{
	debugLogA(__FUNCTION__ ": entering");

	ptrA token(getStringA("TokenSecret"));

	PollParam param;
	param.errors = 0;
	param.errorsLimit = getByte("PollingErrorsLimit", STEAM_API_POLLING_ERRORS_LIMIT);
	while (IsOnline() && param.errors < param.errorsLimit) {
		// request->nlc = m_pollingConnection;
		ptrA umqId(getStringA("UMQID"));
		UINT32 messageId = getDword("MessageID", 0);
		SendRequest(
			new PollRequest(token, umqId, messageId, IdleSeconds()),
			&CSteamProto::OnGotPoll,
			(void*)&param);
	}

	if (IsOnline()) {
		debugLogA(__FUNCTION__ ": unexpected termination; switching protocol to offline");
		SetStatus(ID_STATUS_OFFLINE);
	}

	m_hPollingThread = nullptr;
	debugLogA(__FUNCTION__ ": leaving");
}
