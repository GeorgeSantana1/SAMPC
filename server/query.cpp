/*

	SA:MP Multiplayer Modification
	Copyright 2004-2005 SA:MP Team

	Version: $Id: query.cpp,v 1.16 2006/05/08 13:28:46 kyeman Exp $

*/

#include "main.h"

extern RakServerInterface* pRak;

sockaddr_in to;
SOCKET cur_sock = INVALID_SOCKET;
char* cur_data = NULL;
int	cur_datalen = 0;

bool bRconSocketReply = false;
void RconSocketReply(char* szMessage)
{
	if (bRconSocketReply)
	{
		char* newdata = (char*)malloc(cur_datalen + strlen(szMessage) + sizeof(WORD));
		memcpy(newdata, cur_data, cur_datalen);
		char* keep_ptr = newdata;
		*(WORD*)newdata = (WORD)strlen(szMessage);
		newdata += sizeof(WORD);
		memcpy(newdata, szMessage, strlen(szMessage));
		sendto(cur_sock, keep_ptr, (int)(newdata - keep_ptr), 0, (sockaddr*)&to, sizeof(to));
		free(keep_ptr);
	}
}

int ProcessQueryPacket(unsigned int binaryAddress, unsigned short port, char* data, int length, SOCKET s)
{
	// Expecting atleast 10 bytes long data, starting first 4 bytes with "SAMP"
	if (length >= 11 && *(unsigned int*)data == 0x504D4153)
	{
		to.sin_family = AF_INET;
		to.sin_port = htons(port);
		to.sin_addr.s_addr = binaryAddress;
				
		in_addr in;
		in.s_addr = binaryAddress;
				
		// Tell the user someone sent a request, if "logqueries" enabled
		if (bQueryLogging)
			logprintf("[query:%c] from %s:%d", data[10], inet_ntoa(in), port);
				
		// Data was in fact query request 
		switch (data[10])
		{
			case 'c': //players
			{
				WORD wPlayerCount = 0;
				CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();

				if (pPlayerPool) {
					for (int i = 0; i < MAX_PLAYERS; i++)
						if (pPlayerPool->GetSlotState(i))
							wPlayerCount++;
				}

				char* newdata = (char*)malloc(29 + (wPlayerCount * (MAX_PLAYER_NAME + 13)));
				char* keep_ptr = newdata;

				// Previous Data
				memcpy(newdata, data, 11);
				newdata += 11;

				// Player Count
				memcpy(newdata, &wPlayerCount, sizeof(WORD));
				newdata += sizeof(WORD);

				if (pPlayerPool)
				{
					char* szName;
					BYTE byteNameLen;
					DWORD dwScore;

					for (int r = 0; r < MAX_PLAYERS; r++)
					{
						if (pPlayerPool->GetSlotState(r))
						{
							CPlayer* pPlayers = pNetGame->GetPlayerPool()->GetAt(r);
							szName = (char*)pPlayers->GetName();
							byteNameLen = (BYTE)strlen(szName);
							memcpy(newdata, &byteNameLen, sizeof(BYTE));
							newdata += sizeof(BYTE);
							memcpy(newdata, szName, byteNameLen);
							newdata += byteNameLen;
							dwScore = pPlayers->m_iScore;
							memcpy(newdata, &dwScore, sizeof(DWORD));
							newdata += sizeof(DWORD);
							
						}
					}
				}
				
				sendto(s, keep_ptr, (int)(newdata - keep_ptr), 0, (sockaddr*)&to, sizeof(to));
				free(keep_ptr);
				break;
			}

			case 'd': //detailed player list 
			{
				WORD wPlayerCount = 0;
				CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
				if (pPlayerPool) {
					for (int i = 0; i < MAX_PLAYERS; i++)
						if (pPlayerPool->GetSlotState(i))
							wPlayerCount++;
				}

				char* newdata = (char*)malloc(34 + (wPlayerCount * (MAX_PLAYER_NAME + 13)));
				char* keep_ptr = newdata;
						
				// Previous Data
				memcpy(newdata, data, 11);
				newdata += 11;
						
				// Player Count
				memcpy(newdata, &wPlayerCount, sizeof(WORD));
				newdata += sizeof(WORD);

				if (pPlayerPool)
				{
					char* szName;
					BYTE byteNameLen;
					DWORD dwScore, dwPing;

					for (int r = 0; r < MAX_PLAYERS; r++)
					{
						if (pPlayerPool->GetSlotState(r))
						{
							CPlayer* pPlayers = pNetGame->GetPlayerPool()->GetAt(r);
							memcpy(newdata, &r, sizeof(BYTE));
							newdata += sizeof(BYTE);
							szName = (char*)pPlayers->GetName();
							byteNameLen = (BYTE)strlen(szName);
							memcpy(newdata, &byteNameLen, sizeof(BYTE));
							newdata += sizeof(BYTE);
							memcpy(newdata, szName, byteNameLen);
							newdata += byteNameLen;
							dwScore = pPlayers->m_iScore;
							memcpy(newdata, &dwScore, sizeof(DWORD));
							newdata += sizeof(DWORD);
							dwPing = pRak->GetLastPing(pRak->GetPlayerIDFromIndex(r));
							memcpy(newdata, &dwPing, sizeof(DWORD));
							newdata += sizeof(DWORD);
						}
					}
				}

				sendto(s, keep_ptr, (int)(newdata - keep_ptr), 0, (sockaddr*)&to, sizeof(to));
				free(keep_ptr);
				break;
			}

			case 'i': //info
			{
				char* szHostname = pConsole->GetStringVariable("hostname");
				DWORD dwHostnameLen = strlen(szHostname);
				if (dwHostnameLen > 50) dwHostnameLen = 50;
					
				char* szGameMode = pConsole->GetStringVariable("gamemodetext");
				DWORD dwGameModeLen = strlen(szGameMode);
				if (dwGameModeLen > 30) dwGameModeLen = 30;
					
				char* szLanguageName = pConsole->GetStringVariable("language");
				DWORD dwLanguageNameLen = strlen(szLanguageName);
				if (dwLanguageNameLen > 30) dwLanguageNameLen = 30;

				WORD wPlayerCount = 0;
				CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
					if (pPlayerPool)
						for (int i = 0; i < MAX_PLAYERS; i++)
							if (pPlayerPool->GetSlotState(i))
								wPlayerCount++;

				WORD wMaxPlayers = pConsole->GetIntVariable("maxplayers");
					
				BYTE byteIsPassworded = pConsole->GetStringVariable("password")[0] != 0;

				DWORD datalen = 28;	
				datalen += dwHostnameLen;
				datalen += dwGameModeLen;
				datalen += dwLanguageNameLen;
					
				char* newdata = (char*)malloc(datalen);
				char* keep_ptr = newdata;

				// Previous Data
				memcpy(newdata, data, 11);
				newdata += 11;

				// IsPassworded
				memcpy(newdata, &byteIsPassworded, sizeof(BYTE));
				newdata += sizeof(BYTE);
					
				// Player Count
				memcpy(newdata, &wPlayerCount, sizeof(WORD));
				newdata += sizeof(WORD);

				// Max Players
				memcpy(newdata, &wMaxPlayers, sizeof(WORD));
				newdata += sizeof(WORD);
					
				// Hostname
				memcpy(newdata, &dwHostnameLen, sizeof(DWORD));
				newdata += sizeof(DWORD);
				memcpy(newdata, szHostname, dwHostnameLen);
				newdata += dwHostnameLen;
					
				// Game Mode
				memcpy(newdata, &dwGameModeLen, sizeof(DWORD));
				newdata += sizeof(DWORD);
				memcpy(newdata, szGameMode, dwGameModeLen);
				newdata += dwGameModeLen;

				// Language Name
				memcpy(newdata, &dwLanguageNameLen, sizeof(DWORD));
				newdata += sizeof(DWORD);
				memcpy(newdata, szLanguageName, dwLanguageNameLen);
				newdata += dwLanguageNameLen;
					
				sendto(s, keep_ptr, datalen, 0, (sockaddr*)&to, sizeof(to));
				free(keep_ptr);
				break;
			}

			case 'p': //ping
			{
				if (length == 15)
					sendto(s, data, 15, 0, (sockaddr*)&to, sizeof(to));
				break;
			}

			case 'r': //rules
			{
				pConsole->SendRules(s, data, (sockaddr_in*)&to, sizeof(to));
				break;
			}

			case 'x': //rcon
			{
				char* szPassword = NULL;
				WORD wStrLen = 0;
				cur_sock = s;
				cur_data = data;
				cur_datalen = 11;
				int tmp_datalen = cur_datalen;
						
				data += cur_datalen;
				tmp_datalen += sizeof(WORD);
				if (length < tmp_datalen) {
					cur_datalen = 0;
					cur_data = NULL;
					cur_sock = INVALID_SOCKET;
				}

				wStrLen = *(WORD*)data;
				data += sizeof(WORD);
				tmp_datalen += wStrLen;
				if (length < tmp_datalen) {
					cur_datalen = 0;
					cur_data = NULL;
					cur_sock = INVALID_SOCKET;
				}

				szPassword = (char*)malloc(wStrLen + 1);
				memcpy(szPassword, data, wStrLen);
				szPassword[wStrLen] = 0;
				data += wStrLen;

				if (!strcmp(szPassword, pConsole->GetStringVariable("rcon_password")))
				{
					tmp_datalen += sizeof(WORD);
					if (length < tmp_datalen)
					{
							free(szPassword);
							cur_datalen = 0;
							cur_data = NULL;
							cur_sock = INVALID_SOCKET;
					}
	
					wStrLen = *(WORD*)data;
					data += sizeof(WORD);
					tmp_datalen += wStrLen;
					if (length < tmp_datalen)
					{
						free(szPassword);
								
						cur_datalen = 0;
						cur_data = NULL;
						cur_sock = INVALID_SOCKET;
					}

					char* szCommand = (char*)malloc(wStrLen + 1);
					memcpy(szCommand, data, wStrLen);
					szCommand[wStrLen] = 0;

					if (pConsole)
					{
						bRconSocketReply = true;
						pConsole->Execute(szCommand);
						bRconSocketReply = false;
					}

					free(szCommand);
				}
				else
				{
					logprintf("BAD RCON ATTEMPT BY: %s", inet_ntoa(in));

					bRconSocketReply = true;
					RconSocketReply("Invalid RCON password.");
					bRconSocketReply = false;
				}

				free(szPassword);
				break;
			} 
		}
		return 1;
	}
	return 0;
}
