#define _WINSOCKAPI_
#include <io.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

#include <API/ARK/Ark.h>
#include "TebexInfo.h"
#include "TebexSecret.h"
#include "TebexForcecheck.h"
#include "TebexOfflineCommands.h"
#include "TebexOnlineCommands.h"
#include "TebexArk.h"
#include "Config.cpp"

#pragma comment(lib, "lib/ArkApi.lib")


void Load()
{	
	TebexArk *plugin = new TebexArk();
	
	ArkApi::GetCommands().AddConsoleCommand("tebex:info", [plugin](APlayerController *player, FString *command, bool) {
		TebexInfo::Call(plugin);
	});
	ArkApi::GetCommands().AddRconCommand("tebex:info", [plugin](RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*) {
		TebexInfo::Call(plugin);
	});

	ArkApi::GetCommands().AddConsoleCommand("tebex:forcecheck", [plugin](APlayerController *player, FString *command, bool) {
		TebexForcecheck::Call(plugin);
	});
	ArkApi::GetCommands().AddRconCommand("tebex:forcecheck", [plugin](RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*) {
		TebexForcecheck::Call(plugin);
	});

	ArkApi::GetCommands().AddConsoleCommand("tebex:secret", [plugin](APlayerController *player, FString *command, bool) {
		TebexSecret::Call(plugin, *command);
	});
	ArkApi::GetCommands().AddRconCommand("tebex:secret", [plugin](RCONClientConnection* rcon_connection, RCONPacket* rcon_packet, UWorld*) {
		TebexSecret::Call(plugin, rcon_packet->Body);
	});

	ArkApi::GetCommands().AddOnChatMessageCallback("chatcallback", [plugin](AShooterPlayerController* player, FString* message, EChatSendMode::Type mode, bool spamCheck, bool cmdExecuted) {
		if (message->TrimStartAndEnd() == plugin->getConfig().buyCommand) {
			player->ServerSendChatMessage(&FString::Format("To buy packages from our webstore, please visit {0}", plugin->getWebstore().domain.ToString()), EChatSendMode::LocalChat);
		}
		return false;
	});

	plugin->logWarning("Loading Config...");
	plugin->readConfig();
	tebexConfig::Config tmpconfig;
	tmpconfig = plugin->getConfig();
		
	if (tmpconfig.secret.ToString() == "") {
		plugin->logError("You have not yet defined your secret key. Use /tebex:secret <secret> to define your key");
	}
	else
	{
		TebexInfo::Call(plugin);
	}

	

	ArkApi::GetCommands().AddOnTimerCallback("commandChecker", [plugin]() {
		std::list<TSharedRef<IHttpRequest>> requests = plugin->getRequests();
		if (requests.size() > 0) {
			auto request = requests.front();
			switch (request->GetStatus())
			{
			case EHttpRequestStatus::Succeeded:
			{
				FString handler = *request->GetHeader(&FString(""), &FString("X-Buycraft-Handler"));
				std::string handlerName = handler.ToString();
				if (handlerName == "TebexInfo") {
					TebexInfo::ApiCallback(plugin, request);
				}
				else if (handlerName == "TebexOfflineCommands") {
					TebexOfflineCommands::ApiCallback(plugin, request);
				}
				else if (handlerName == "TebexSecret") {
					TebexSecret::ApiCallback(plugin, request);
				}
				else if (handlerName == "TebexForcecheck") {
					TebexForcecheck::ApiCallback(plugin, request);
				}
				else if (handlerName == "TebexDeleteCommands") {
					TebexDeleteCommands::ApiCallback(plugin, request);
				}
				else if (handlerName == "TebexOnlineCommands") {
					TebexOnlineCommands::ApiCallback(plugin, request);
				}
				plugin->removeRequestFromQueue();
			}
			break;

			case EHttpRequestStatus::NotStarted:
			{
				plugin->logWarning("Waiting for HTTP response...");
			}
			break;
			case EHttpRequestStatus::Failed:
			{
				plugin->logError("Unable to process API request");
				plugin->removeRequestFromQueue();
			}
			break;
			}
		}

		if (plugin->doCheck()) {
			plugin->loadServer();
			TebexForcecheck::Call(plugin);
		}
	});
	
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Load();
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
