#pragma once

#include "TebexArk.h"

#include <Requests.h>

class TebexEvents {
public:
	enum class EventType {
		PlayerJoin,
		PlayerLeave
	};

	static void AddPlayerEvent(AShooterPlayerController* player, EventType eventType);
	
	static void SendEvents(TebexArk* plugin);

private:
	struct PlayerEventData {
		uint64 steamId;
		FString name;
		EventType type;
		std::chrono::system_clock::time_point event_date;
	};

	static std::string BuildPayload();
	static std::string GetEventName(EventType eventType);
	static std::string GetDateStr(const std::chrono::system_clock::time_point& date);

	static void ApiCallback(TebexArk* plugin, std::string responseText);
	
	inline static TArray<std::unique_ptr<PlayerEventData>> playerEvents_;
};

inline void TebexEvents::AddPlayerEvent(AShooterPlayerController* player, EventType eventType) {
	const uint64 steamId = ArkApi::IApiUtils::GetSteamIdFromController(player);
	const FString steamName = ArkApi::IApiUtils::GetSteamName(player);

	const auto now = std::chrono::system_clock::now();
	
	playerEvents_.Add(std::make_unique<PlayerEventData>(PlayerEventData{
		steamId, steamName, eventType, now
	}));
}

inline std::string TebexEvents::BuildPayload() {
	if (playerEvents_.Num() == 0)
		return "";
	
	auto jsonObjects = nlohmann::json::array();
	
	for (const auto& eventData : playerEvents_)	{
		auto object = nlohmann::json::object();

		object["username_id"] = std::to_string(eventData->steamId);
		object["username"] = ArkApi::Tools::Utf8Encode(*eventData->name);
		object["event_type"] = GetEventName(eventData->type);
		object["event_date"] = GetDateStr(eventData->event_date);
		object["ip"] = "0.0.0.0";
		
		jsonObjects.push_back(object);
	}

	return jsonObjects.dump();
}

inline std::string TebexEvents::GetEventName(EventType eventType) {
	switch(eventType) {
	case EventType::PlayerJoin:
		return "server.join";
	case EventType::PlayerLeave:
		return "server.leave";
	}

	return "Unknown";
}

inline std::string TebexEvents::GetDateStr(const std::chrono::system_clock::time_point& date) {
	std::time_t now_t = std::chrono::system_clock::to_time_t(date);

	tm timeTm{};
	localtime_s(&timeTm, &now_t);
	
	std::stringstream ss;
	ss << std::put_time(&timeTm, "%F %T");
	
	return ss.str();
}

inline void TebexEvents::SendEvents(TebexArk* plugin) {	
	const std::string url = (plugin->getConfig().baseUrl + "/events").ToString();
	std::vector<std::string> headers{
		fmt::format("X-Buycraft-Secret: {}", plugin->getConfig().secret.ToString()),
		"X-Buycraft-Handler: TebexEvents",
		"Content-Type: application/json"
	};

	const std::string payload = BuildPayload();
	if (payload.empty())
		return;

	//plugin->logWarning("Sending events:");
	//plugin->logWarning(payload.c_str());
	
	const bool result = API::Requests::Get().CreatePostRequest(url, [plugin](bool success, std::string response) {
		if (!success) {
			plugin->logWarning("Unable to process API request");
			return;
		}

		ApiCallback(plugin, response);
	}, payload, move(headers));
	if (!result) {
		plugin->logWarning("Call failed");
	}
	else {
		// Clear array so we don't send same events again
		playerEvents_.Empty();
	}
}

inline void TebexEvents::ApiCallback(TebexArk* plugin, std::string responseText) {
	plugin->logWarning(responseText.c_str());
}
