#include "WorldTimeAPI.h"

#ifdef ARDUINO

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

//On ESP8266 there is missing String(const char* cstr, unsigned int length) constructor

class StringE2 : public String {
public:
	using String::String;

	StringE2(const char* cstr, unsigned int length) {
		init();
		if (cstr)
			copy(cstr, length);
	}
};


#elif defined(ESP32)
#include <WiFi.h>
#include <HTTPClient.h>
#endif

#endif // ARDUINO


void WorldTimeAPIResult::clear() {
	timezone[0] = 0;
	abbreviation[0] = 0;
#if (defined(ESP8266) || defined(ESP32)) && defined(ARDUINO)
	client_ip.clear();
#else
	client_ip[0] = 0;
#endif // defined(ESP8266) || defined(ESP32)
	datetime = DateTimeTZSysSync::Zero;
	httpCode = WorldTimeAPI_HttpCode::WTA_HTTP_NO_CODE;
	error = "";
}

#ifdef ARDUINO
WorldTimeAPI_HttpCode WorldTimeAPI::getListOfTimeZones(String& list, const char* tz) {
	String url = URL_TimeZone;
#else
WorldTimeAPI_HttpCode WorldTimeAPI::getListOfTimeZones(std::string& list, const char* tz) {
	std::string url = URL_TimeZone;
#endif // !ARDUINO
	if (tz != NULL) {
		url += '/';
		url += tz;
	}
	url += ".txt";

	int httpCode = requestGET(url.c_str(), list);
	if (list.length() > 6 && strncmp("abbrev", list.c_str(), 6) == 0 && tz != NULL) {
		//Time zone info was get, so return only one time zone name
		list = tz;
	}
	return (WorldTimeAPI_HttpCode)httpCode;
} 

const WorldTimeAPIResult& WorldTimeAPI::getByTimeZone(const char* tz) {
	if (tz == NULL) {
		//tz cannot be NULL
		lastRes.clear();
		lastRes.httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_ARGUMENT_ERROR;
		return lastRes;
	}
#ifdef ARDUINO
	String url = URL_TimeZone;
#else
	std::string url = URL_TimeZone;
#endif // ARDUINO
	url += '/';
	url += tz;

	return getAndParseTZ(url.c_str());
}

const WorldTimeAPIResult& WorldTimeAPI::getByIP(const char* IP) {
#ifdef ARDUINO
	String url = URL_IP;
#else
	std::string url = URL_IP;
#endif // ARDUINO
	if (IP != NULL) {
		url += '/';
		url += IP;
	}

	return getAndParseTZ(url.c_str());
}

#if (defined(ESP8266) || defined(ESP32)) && defined(ARDUINO)
const WorldTimeAPIResult& WorldTimeAPI::getByIP(const IPAddress& IP) {
	lastRes.clear();
	if (!(IP.isV4() && IP.isSet())) {
		//Unset IP or IPV6
		lastRes.httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_ARGUMENT_ERROR;
		return lastRes;
	}

	String url = URL_IP;
	url += '/';
	url += IP.toString();

	return getAndParseTZ(url.c_str());
}
#endif // !SJSONP_UNDER_OS


const WorldTimeAPIResult& WorldTimeAPI::getAndParseTZ(const char* url) {
	lastRes.clear();
	int httpCode;
#if defined(ARDUINO)
	String response;
#else
	std::string response;
#endif // !defined(ARDUINO)

	httpCode = requestGET(url, response);
	lastRes.httpCode = (WorldTimeAPI_HttpCode)httpCode;

	//Serial.println(response);

	if (lastRes.httpCode == WorldTimeAPI_HttpCode::WTA_HTTP_CODE_OK) {
		//GET request successfull
		WorldTimeAPIResHelper resHelper(&lastRes);
		SimpleJSONTextParser parser;
		parser.onItemFound = jsonItemTZ;
		parser.onTextItemFound = jsonTextTZ;
		parser.onObjArrFound = jsonControlTZ;
//{"abbreviation":"CEST","client_ip":"185.142.49.50","datetime":"2022-06-16T13:57:27.659132+02:00","day_of_week":4,"day_of_year":167,"dst":true,"dst_from":"2022-03-27T01:00:00+00:00","dst_offset":3600,"dst_until":"2022-10-30T01:00:00+00:00","raw_offset":3600,"timezone":"Europe/Bratislava","unixtime":1655380647,"utc_datetime":"2022-06-16T11:57:27.659132+00:00","utc_offset":"+02:00","week_number":24}
		int result = parser.parseJSON(response.c_str(), (int)response.length(), &resHelper);
		//Serial.println(result);
		if (!resHelper.foundFlags.allValidFound()) {
			if (lastRes.httpCode == WorldTimeAPI_HttpCode::WTA_HTTP_CODE_OK) lastRes.httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_FIELD_MISSING;
		}
		else if (result <= 0) {
			if (lastRes.httpCode == WorldTimeAPI_HttpCode::WTA_HTTP_CODE_OK) lastRes.httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_WRONG_RESPONSE;
		}
		else {
			//Parsing OK
			DSTAdjustment adj;
			bool isDST = false;
			if (!resHelper.dst_null) {
				//Creating fake DST adjustment - TODO
				//TODO there may be problem at winter or at south hemisphere

				resHelper.dst_from += resHelper.tz.getTimeZoneOffset();
				date_s tmp = resHelper.dst_from.getDateStruct();
				DSTTransitionRule startRule = DSTTransitionRule::Date(resHelper.dst_from.getHours(), tmp.month, tmp.day);

				resHelper.dst_until += resHelper.tz.getTimeZoneOffset() + (((int64_t)resHelper.dst_offset) * SECOND);
				tmp = resHelper.dst_until.getDateStruct();
				DSTTransitionRule endRule = DSTTransitionRule::Date(resHelper.dst_until.getHours(), tmp.month, tmp.day);

				isDST = resHelper.dst;
				adj = DSTAdjustment::fromTotalMinutesOffset(startRule, endRule, resHelper.dst_offset / 60, isDST);
			}

			resHelper.unixtime += resHelper.tz.getTimeZoneOffset() + adj.getDSTOffset();
			lastRes.datetime = DateTimeTZSysSync(resHelper.unixtime, resHelper.tz, adj, isDST);
		}
	}
	else if(lastRes.httpCode > WorldTimeAPI_HttpCode::WTA_HTTP_NO_CODE && response.length() > 0) {
		//Trying to parse error
		WorldTimeAPIResHelper resHelper(&lastRes);
		SimpleJSONTextParser parser;
		parser.onItemFound = jsonItemERR;
		parser.onTextItemFound = jsonTextERR;
		parser.onObjArrFound = jsonControlTZ;
		parser.parseJSON(response.c_str(), (int)response.length(), &resHelper);
	}
	return lastRes;
}

bool WorldTimeAPI::jsonItemTZ(JSONItemType type, const char* key, int keyLength, const SimpleJSONTextParser::Number& parsedVal, int depth, int index, void* owner_ptr) {
	if (depth != 1) return true; //We need only depth 1 here

	WorldTimeAPIResHelper& res = *reinterpret_cast<WorldTimeAPIResHelper*>(owner_ptr);
	if (keyLength == 12 && strncmp("abbreviation", key, 12) == 0) {
		if (res.foundFlags.abbreviation_found) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_FIELD_DOUBLE;
		}
		else {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_WRONG_VALUE_TYPE;
		}
		res.foundFlags.abbreviation_found = true;
		return false; //ERROR
	}
	else if (keyLength == 9 && strncmp("client_ip", key, 9) == 0) {
		if (res.foundFlags.client_ip_found) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_FIELD_DOUBLE;
		}
		else {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_WRONG_VALUE_TYPE;
		}
		res.foundFlags.client_ip_found = true;
		return false; //ERROR
	}
	else if (keyLength == 3 && strncmp("dst", key, 3) == 0) {
		if (res.foundFlags.dst_found) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_FIELD_DOUBLE;
			return false; //Same key found
		}
		res.foundFlags.dst_found = true;
		if (parsedVal.Type == SimpleJSONTextParser::Number::NT_Bool) {
			res.dst = parsedVal.Value.Bool;
		}
		else if (parsedVal.Type == SimpleJSONTextParser::Number::NT_Null) {
			res.dst_null = true; //Null found
		}
		else {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_WRONG_VALUE_TYPE;
			return false; //ERROR: wrong type
		}
	}
	else if (keyLength == 8 && strncmp("dst_from", key, 8) == 0) { //Can return null
		if (res.foundFlags.dst_from_found) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_FIELD_DOUBLE;
			return false; //Same key found
		}
		res.foundFlags.dst_from_found = true;
		if (parsedVal.Type == SimpleJSONTextParser::Number::NT_Null) {
			res.dst_null = true; //Null found
		}
		else {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_WRONG_VALUE_TYPE;
			return false; //ERROR: wrong type
		}
	}
	else if (keyLength == 10 && strncmp("dst_offset", key, 10) == 0) {
		if (res.foundFlags.dst_offset_found) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_FIELD_DOUBLE;
			return false; //Same key found
		}
		res.foundFlags.dst_offset_found = true;
		if (parsedVal.Type >= SimpleJSONTextParser::Number::NT_Int8 && parsedVal.Type <= SimpleJSONTextParser::Number::NT_Uint16) {
			//Valid number found
			res.dst_offset = parsedVal.Value.Int16;
		}
		else {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_WRONG_VALUE_TYPE;
			return false; //ERROR: wrong type
		}
	}
	else if (keyLength == 9 && strncmp("dst_until", key, 9) == 0) { //Can return null
		if (res.foundFlags.dst_until_found) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_FIELD_DOUBLE;
			return false; //Same key found
		}
		res.foundFlags.dst_until_found = true;
		if (parsedVal.Type == SimpleJSONTextParser::Number::NT_Null) {
			res.dst_null = true; //Null found
		}
		else {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_WRONG_VALUE_TYPE;
			return false; //ERROR: wrong type
		}
	}
	else if (keyLength == 10 && strncmp("raw_offset", key, 10) == 0) {
		if (res.foundFlags.raw_offset_found) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_FIELD_DOUBLE;
			return false; //Same key found
		}
		res.foundFlags.raw_offset_found = true;
		if (parsedVal.Type >= SimpleJSONTextParser::Number::NT_Int8 && parsedVal.Type <= SimpleJSONTextParser::Number::NT_Uint16) {
			//Valid number found
			res.tz = TimeZone::fromTotalMinutesOffset(parsedVal.Value.Int16/60);
		}
		else {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_WRONG_VALUE_TYPE;
			return false; //ERROR: wrong type
		}
	}
	else if (keyLength == 8 && strncmp("timezone", key, 8) == 0) {
		if (res.foundFlags.timezone_found) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_FIELD_DOUBLE;
		}
		else {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_WRONG_VALUE_TYPE;
		}
		res.foundFlags.timezone_found = true;
		return false; //ERROR
	}
	else if (keyLength == 12 && strncmp("utc_datetime", key, 12) == 0) {
		if (res.foundFlags.unixtime_found) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_FIELD_DOUBLE;
		}
		else {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_WRONG_VALUE_TYPE;
		}
		res.foundFlags.unixtime_found = true;
		return false; //ERROR
	}
	else if (keyLength == 5 && strncmp("error", key, 5) == 0) {
		//WorldTimeAPI sent error
		res.foundFlags.error_found = true;
		if (res.result_ptr->httpCode == WorldTimeAPI_HttpCode::WTA_HTTP_NO_CODE) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_ERROR_RESPONSE;
		}
		return false; //ERROR
	}
	return true;
}

bool WorldTimeAPI::jsonTextTZ(const char* key, int keyLength, const char* value, int valueLength, int depth, int index, void* owner_ptr) {
	if (depth != 1) return true; //We need only depth 1 here

	WorldTimeAPIResHelper& res = *reinterpret_cast<WorldTimeAPIResHelper*>(owner_ptr);
	if (keyLength == 12 && strncmp("abbreviation", key, 12) == 0) {
		if (res.foundFlags.abbreviation_found) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_FIELD_DOUBLE;
			return false; //Same key found
		}
		res.foundFlags.abbreviation_found = true;
		SimpleJSONTextParser::unescapeAndCopy(res.result_ptr->abbreviation, sizeof(WorldTimeAPIResult::abbreviation) / sizeof(char), value, valueLength);
	}
	else if (keyLength == 9 && strncmp("client_ip", key, 9) == 0) {
		if (res.foundFlags.client_ip_found) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_FIELD_DOUBLE;
			return false; //Same key found
		}
		res.foundFlags.client_ip_found = true;
#if (defined(ESP8266) || defined(ESP32)) && defined(ARDUINO)
		char client_ip[WTAPI_TZ_CLIENT_IP_SIZE];
		SimpleJSONTextParser::unescapeAndCopy(client_ip, WTAPI_TZ_CLIENT_IP_SIZE, value, valueLength); //Copy and unescape IP
		//Parse IP
		//Serial.println(client_ip);
		if (!res.result_ptr->client_ip.fromString(client_ip)) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_WRONG_VALUE_FORMAT;
			return false; //ERROR parsing failed
		}
#else
		SimpleJSONTextParser::unescapeAndCopy(res.result_ptr->client_ip, sizeof(WorldTimeAPIResult::client_ip) / sizeof(char), value, valueLength);
#endif // (defined(ESP8266) || defined(ESP32)) && defined(ARDUINO)
	}
	else if (keyLength == 3 && strncmp("dst", key, 3) == 0) {
		if (res.foundFlags.dst_found) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_FIELD_DOUBLE;
		}
		else {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_WRONG_VALUE_TYPE;
		}
		res.foundFlags.dst_found = true;
		return false; //ERROR
	}
	else if (keyLength == 8 && strncmp("dst_from", key, 8) == 0) {
		if (res.foundFlags.dst_from_found) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_FIELD_DOUBLE;
			return false; //Same key found
		}
		res.foundFlags.dst_from_found = true;
		DateTime tmp;
		if (res.dst_from.parse(value, valueLength, "y-M-dTHH:mm:sszzz", true) <= 0) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_WRONG_VALUE_FORMAT;
			return false; //Parsing error
		}
	}
	else if (keyLength == 10 && strncmp("dst_offset", key, 10) == 0) {
		if (res.foundFlags.dst_offset_found) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_FIELD_DOUBLE;
		}
		else {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_WRONG_VALUE_TYPE;
		}
		res.foundFlags.dst_offset_found = true;
		return false; //ERROR
	}
	else if (keyLength == 9 && strncmp("dst_until", key, 9) == 0) {
		if (res.foundFlags.dst_until_found) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_FIELD_DOUBLE;
			return false; //Same key found
		}
		res.foundFlags.dst_until_found = true;
		if (res.dst_until.parse(value, valueLength, "y-M-dTHH:mm:sszzz", true) <= 0) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_WRONG_VALUE_FORMAT;
			return false; //Parsing error
		}
	}
	else if (keyLength == 10 && strncmp("raw_offset", key, 10) == 0) {
		if (res.foundFlags.raw_offset_found) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_FIELD_DOUBLE;
		}
		else {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_WRONG_VALUE_TYPE;
		}
		res.foundFlags.raw_offset_found = true;
		return false; //ERROR
	}
	else if (keyLength == 8 && strncmp("timezone", key, 8) == 0) {
		if (res.foundFlags.timezone_found) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_FIELD_DOUBLE;
			return false; //Same key found
		}
		res.foundFlags.timezone_found = true;
		SimpleJSONTextParser::unescapeAndCopy(res.result_ptr->timezone, sizeof(WorldTimeAPIResult::timezone) / sizeof(char), value, valueLength);
	}
	else if (keyLength == 12 && strncmp("utc_datetime", key, 12) == 0) {
		if (res.foundFlags.unixtime_found) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_FIELD_DOUBLE;
			return false; //Same key found
		}
		res.foundFlags.unixtime_found = true;
		if (res.unixtime.parse(value, valueLength, "y-M-dTHH:mm:ss.FFFFFFzzz", true) <= 0) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_WRONG_VALUE_FORMAT;
			return false; //Parsing error
		}
	}
	else if (keyLength == 5 && strncmp("error", key, 5) == 0) {
		//WorldTimeAPI sent error
		res.foundFlags.error_found = true;
		if (res.result_ptr->httpCode == WorldTimeAPI_HttpCode::WTA_HTTP_NO_CODE || res.result_ptr->httpCode == WorldTimeAPI_HttpCode::WTA_HTTP_CODE_OK) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_ERROR_RESPONSE;
		}
#ifdef ARDUINO
#ifdef ESP8266
		res.result_ptr->error = (String)StringE2(value, valueLength);
#else
		res.result_ptr->error = String(value, valueLength);
#endif //ESP8266
#else
		res.result_ptr->error = std::string(value, valueLength);
#endif // ARDUINO
		return false; //ERROR found
	}
	return true;
}

bool WorldTimeAPI::jsonControlTZ(JSONItemType type, const char* key, int keyLength, int depth, int index, void* owner_ptr) {
	bool isArrJSON = (type == JSONItemType::JIT_ArrayBegin && depth == 0);
	if (isArrJSON) {
		WorldTimeAPIResHelper& res = *reinterpret_cast<WorldTimeAPIResHelper*>(owner_ptr);
		res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_PARTIAL_TIMEZONE;
	}

	return !isArrJSON; //Returns true when JSON is object, false when JSON is array
}

bool WorldTimeAPI::jsonItemERR(JSONItemType type, const char* key, int keyLength, const SimpleJSONTextParser::Number& parsedVal, int depth, int index, void* owner_ptr) {
	if (depth != 1) return true; //We need only depth 1 here

	WorldTimeAPIResHelper& res = *reinterpret_cast<WorldTimeAPIResHelper*>(owner_ptr);
	if (keyLength == 5 && strncmp("error", key, 5) == 0) {
		//WorldTimeAPI sent error
		res.foundFlags.error_found = true;
		if (res.result_ptr->httpCode == WorldTimeAPI_HttpCode::WTA_HTTP_NO_CODE) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_ERROR_RESPONSE;
		}
		return false; //ERROR found
	}
	return true;
}

bool WorldTimeAPI::jsonTextERR(const char* key, int keyLength, const char* value, int valueLength, int depth, int index, void* owner_ptr) {
	if (depth != 1) return true; //We need only depth 1 here

	WorldTimeAPIResHelper& res = *reinterpret_cast<WorldTimeAPIResHelper*>(owner_ptr);
	if (keyLength == 5 && strncmp("error", key, 5) == 0) {
		//WorldTimeAPI sent error
		res.foundFlags.error_found = true;
		if (res.result_ptr->httpCode == WorldTimeAPI_HttpCode::WTA_HTTP_NO_CODE || res.result_ptr->httpCode == WorldTimeAPI_HttpCode::WTA_HTTP_CODE_OK) {
			res.result_ptr->httpCode = WorldTimeAPI_HttpCode::WTA_ERROR_ERROR_RESPONSE;
		}
#ifdef ARDUINO
#ifdef ESP8266
		res.result_ptr->error = (String)StringE2(value, valueLength);
#else
		res.result_ptr->error = String(value, valueLength);
#endif //ESP8266
#else
		res.result_ptr->error = std::string(value, valueLength);
#endif // ARDUINO
		return false; //ERROR found
	}
	return true;
}



const char* WorldTimeAPI::URL_TimeZone = "http://worldtimeapi.org/api/timezone";
const char* WorldTimeAPI::URL_IP = "http://worldtimeapi.org/api/ip";


#if defined(SJSONP_UNDER_OS)
WorldTimeAPI_HttpCode WorldTimeAPI::requestGET(const char* url, std::string& resp) {
	std::string cmd = "curl -is \"";
	cmd += url;
	cmd += '"';

	resp = ssystem(cmd.c_str());
	int httpCode = 0;

	if (resp.length() > 5 && strncmp("curl:", resp.c_str(), 5) == 0) {
		//CURL error
		resp = "";
		return WorldTimeAPI_HttpCode::WTA_HTTP_ERROR_CONNECTION_FAILED;
	}
	else if (resp.length() > 4 && strncmp("HTTP", resp.c_str(), 4) == 0) {
		//HTTP header found
		size_t respLen = resp.length();
		size_t p1 = resp.find(' ');
		if (p1 != std::string::npos) {
			//Parsing http code
			++p1;
			for (; p1 < respLen && resp[p1] >= '0' && resp[p1] <= '9'; p1++) {
				httpCode *= 10;
				httpCode += resp[p1] - '0';
			}
			p1 = resp.find("\r\n\r\n", p1);
			if (p1 != std::string::npos) {
				resp = resp.substr(p1 + 4);
				return (WorldTimeAPI_HttpCode)httpCode;
			}
		}
	}
	
	//ERROR
	resp = "";
	return WorldTimeAPI_HttpCode::WTA_HTTP_ERROR_CONNECTION_FAILED;
}
#elif defined(ARDUINO)
WorldTimeAPI_HttpCode WorldTimeAPI::requestGET(const char* url, String& resp) {
	resp = "";
	WiFiClient client;
	HTTPClient http;
	http.setTimeout(1000);

	if (http.begin(client, url)) {
		int httpCode = http.GET();

		// httpCode will be negative on error
		if (httpCode > 0) {
			resp = http.getString();
		}
		return (WorldTimeAPI_HttpCode)httpCode;
	}
	else {
		return WorldTimeAPI_HttpCode::WTA_HTTP_ERROR_CONNECTION_FAILED;
	}
}
#endif // !SJSONP_UNDER_OS


#if defined(SJSONP_UNDER_OS)
/**
* @brief Calls command (CMD) and retrieves it's result.
* @param command Command to call.
* @return Returns response to command.
*/
std::string WorldTimeAPI::ssystem(const char* command) {
	char tmpname[L_tmpnam];
	tmpnam_s(tmpname, L_tmpnam);
	std::string scommand = command;
	std::string cmd = scommand + " > \"" + tmpname + "\" 2>&1";
	std::system(cmd.c_str());
	std::ifstream file(tmpname, std::ios::in | std::ios::binary);
	std::stringstream buffer;
	if (file) {
		buffer << file.rdbuf();
	}
	remove(tmpname);
	return buffer.str();
}
#endif // !SJSONP_UNDER_OS
