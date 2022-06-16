/**
 * @file WorldTimeAPI.h
 * @brief This file contains base class cliend side of WorldTimeAPI.
 *
 * @see WorldTimeAPI
 *
 * # Credits
 * @author Matej Fitoš
 * @date Jun 15, 2022
 */

#ifndef WORLD_TIME_API_H_
#define WORLD_TIME_API_H_

#include "DateTime.h"
#include "SimpleJSONParser.h"

#if defined(SJSONP_UNDER_OS)
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>

#elif defined(ARDUINO)

#if defined(ESP8266) || defined(ESP32)
#include <IPAddress.h>
#endif



#endif // !SJSONP_UNDER_OS

#define WTAPI_TZ_NAME_SIZE        (45)
#define WTAPI_TZ_ABR_NAME_SIZE    (8)
#define WTAPI_TZ_CLIENT_IP_SIZE   (3 * 4 + 3 + 1)

//WorldTimeAPI http codes
typedef enum {
	/**
	* Full time zone was not specified and list of time zones was returned.
	* To get that list, call getListOfTimeZones(). This can be set only when calling getByTimeZone().
	*/
	WTA_ERROR_PARTIAL_TIMEZONE = -108,
	/**
	* Wrong argument set.
	*/
	WTA_ERROR_ARGUMENT_ERROR = -107,
	/**
	* Same JSON field found multiple times
	*/
	WTA_ERROR_FIELD_DOUBLE = -106,
	/**
	* Some fields from JSON response are missing
	*/
	WTA_ERROR_FIELD_MISSING = -105,
	/**
	* Some value in JSON response has incorrect format
	*/
	WTA_ERROR_WRONG_VALUE_FORMAT = -104,
	/**
	* Some value in JSON was out of range
	*/
	WTA_ERROR_VALUE_OUT_OF_RANGE = -103,
	/**
	* Some value type in JSON response has differrent type as expected
	*/
	WTA_ERROR_WRONG_VALUE_TYPE = -102,
	/**
	* JSON response parsing failed or wrong response returned
	*/
	WTA_ERROR_WRONG_RESPONSE = -101,
	/**
	* JSON response contains "error" field with error message
	*/
	WTA_ERROR_ERROR_RESPONSE = -100,



	//ESP8266 and ESP32 codes
	WTA_HTTP_ERROR_CONNECTION_FAILED = -1,
	WTA_HTTP_ERROR_SEND_HEADER_FAILED = -2,
	WTA_HTTP_ERROR_SEND_PAYLOAD_FAILED = -3,
	WTA_HTTP_ERROR_NOT_CONNECTED = -4,
	WTA_HTTP_ERROR_CONNECTION_LOST = -5,
	WTA_HTTP_ERROR_NO_STREAM = -6,
	WTA_HTTP_ERROR_NO_HTTP_SERVER = -7,
	WTA_HTTP_ERROR_TOO_LESS_RAM = -8,
	WTA_HTTP_ERROR_ENCODING = -9,
	WTA_HTTP_ERROR_STREAM_WRITE = -10,
	WTA_HTTP_ERROR_READ_TIMEOUT = -11,

	WTA_HTTP_NO_CODE = 0,
	WTA_HTTP_CODE_CONTINUE = 100,
	WTA_HTTP_CODE_SWITCHING_PROTOCOLS = 101,
	WTA_HTTP_CODE_PROCESSING = 102,
	WTA_HTTP_CODE_OK = 200,
	WTA_HTTP_CODE_CREATED = 201,
	WTA_HTTP_CODE_ACCEPTED = 202,
	WTA_HTTP_CODE_NON_AUTHORITATIVE_INFORMATION = 203,
	WTA_HTTP_CODE_NO_CONTENT = 204,
	WTA_HTTP_CODE_RESET_CONTENT = 205,
	WTA_HTTP_CODE_PARTIAL_CONTENT = 206,
	WTA_HTTP_CODE_MULTI_STATUS = 207,
	WTA_HTTP_CODE_ALREADY_REPORTED = 208,
	WTA_HTTP_CODE_IM_USED = 226,
	WTA_HTTP_CODE_MULTIPLE_CHOICES = 300,
	WTA_HTTP_CODE_MOVED_PERMANENTLY = 301,
	WTA_HTTP_CODE_FOUND = 302,
	WTA_HTTP_CODE_SEE_OTHER = 303,
	WTA_HTTP_CODE_NOT_MODIFIED = 304,
	WTA_HTTP_CODE_USE_PROXY = 305,
	WTA_HTTP_CODE_TEMPORARY_REDIRECT = 307,
	WTA_HTTP_CODE_PERMANENT_REDIRECT = 308,
	WTA_HTTP_CODE_BAD_REQUEST = 400,
	WTA_HTTP_CODE_UNAUTHORIZED = 401,
	WTA_HTTP_CODE_PAYMENT_REQUIRED = 402,
	WTA_HTTP_CODE_FORBIDDEN = 403,
	WTA_HTTP_CODE_NOT_FOUND = 404,
	WTA_HTTP_CODE_METHOD_NOT_ALLOWED = 405,
	WTA_HTTP_CODE_NOT_ACCEPTABLE = 406,
	WTA_HTTP_CODE_PROXY_AUTHENTICATION_REQUIRED = 407,
	WTA_HTTP_CODE_REQUEST_TIMEOUT = 408,
	WTA_HTTP_CODE_CONFLICT = 409,
	WTA_HTTP_CODE_GONE = 410,
	WTA_HTTP_CODE_LENGTH_REQUIRED = 411,
	WTA_HTTP_CODE_PRECONDITION_FAILED = 412,
	WTA_HTTP_CODE_PAYLOAD_TOO_LARGE = 413,
	WTA_HTTP_CODE_URI_TOO_LONG = 414,
	WTA_HTTP_CODE_UNSUPPORTED_MEDIA_TYPE = 415,
	WTA_HTTP_CODE_RANGE_NOT_SATISFIABLE = 416,
	WTA_HTTP_CODE_EXPECTATION_FAILED = 417,
	WTA_HTTP_CODE_MISDIRECTED_REQUEST = 421,
	WTA_HTTP_CODE_UNPROCESSABLE_ENTITY = 422,
	WTA_HTTP_CODE_LOCKED = 423,
	WTA_HTTP_CODE_FAILED_DEPENDENCY = 424,
	WTA_HTTP_CODE_UPGRADE_REQUIRED = 426,
	WTA_HTTP_CODE_PRECONDITION_REQUIRED = 428,
	WTA_HTTP_CODE_TOO_MANY_REQUESTS = 429,
	WTA_HTTP_CODE_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
	WTA_HTTP_CODE_INTERNAL_SERVER_ERROR = 500,
	WTA_HTTP_CODE_NOT_IMPLEMENTED = 501,
	WTA_HTTP_CODE_BAD_GATEWAY = 502,
	WTA_HTTP_CODE_SERVICE_UNAVAILABLE = 503,
	WTA_HTTP_CODE_GATEWAY_TIMEOUT = 504,
	WTA_HTTP_CODE_HTTP_VERSION_NOT_SUPPORTED = 505,
	WTA_HTTP_CODE_VARIANT_ALSO_NEGOTIATES = 506,
	WTA_HTTP_CODE_INSUFFICIENT_STORAGE = 507,
	WTA_HTTP_CODE_LOOP_DETECTED = 508,
	WTA_HTTP_CODE_NOT_EXTENDED = 510,
	WTA_HTTP_CODE_NETWORK_AUTHENTICATION_REQUIRED = 511
}WorldTimeAPI_HttpCode;


/**
* @struct WorldTimeAPIResult
* @brief WorldTimeAPI result, which contains time zone and date time informations.
*/
struct WorldTimeAPIResult {
	WorldTimeAPIResult() {
		clear();
	}

	/**
	* @brief Olson time zone name. IANA database identifier.
	*/
	char timezone[WTAPI_TZ_NAME_SIZE];

	/**
	* @brief Abbreviation name of time zone.
	*/
	char abbreviation[WTAPI_TZ_ABR_NAME_SIZE];

#if (defined(ESP8266) || defined(ESP32)) && defined(ARDUINO)
	/**
	* @brief Client public IPv4 IP address. On ESP32 and ESP8266 with arduino core
	* IP address is represented by IPAddress class.
	*/
	IPAddress client_ip;
#else
	/**
	* @brief Client public IPv4 IP address represented by string.
	*/
	char client_ip[WTAPI_TZ_CLIENT_IP_SIZE];
#endif // defined(ESP8266) || defined(ESP32)
	
	/**
	* @brief Current date time, time zone offset and DST adjustment.
	*/
	DateTimeTZSysSync datetime;

	/**
	* @brief HTTP code of operation.
	*/
	WorldTimeAPI_HttpCode httpCode;

#ifdef ARDUINO
	/**
	* @brief Error returned by WorldTimeAPI.
	*/
	String error;
#else
	/**
	* @brief Error returned by WorldTimeAPI.
	*/
	std::string error;
#endif // ARDUINO

	/**
	* @brief True if current response is not valid.
	*/
	inline bool hasError() const {
		return httpCode != WorldTimeAPI_HttpCode::WTA_HTTP_CODE_OK;
	}

	/**
	* @brief Clears result.
	*/
	void clear();
};


/**
* @class WorldTimeAPI
* @brief This class represent client side of <a href="http://worldtimeapi.org/">WorldTimeAPI</a>.
* 
* > WorldTimeAPI is a simple web service which returns the current local time for a given timezone as either plain-text or JSON.
*/
class WorldTimeAPI
{
public:

#ifdef ARDUINO/**
	* @brief Gets list of accepted olson time zones.
	* @warning This method can return string with size up to 7kB, which may use all RAM memory on microcontrollers.
	* @param[out] list Out parameter, which will contain list of all accepted time zones separated by "\r\n".
	* @param[in] tz Part olson time zone, for example to list all time zones in Europe,
	* set it to: "Europe" and it will returns: "Europe/Amsterdam","Europe/Andorra","Europe/Astrakhan", ...
	* If set to NULL, all supported time zones are retrieved.
	* @return Returns HTTP code of result.
	*/
	WorldTimeAPI_HttpCode getListOfTimeZones(String& list, const char* tz = NULL);
#else
	/**
	* @brief Gets list of accepted olson time zones.
	* @param[out] list Out parameter, which will contain list of all accepted time zones separated by "\r\n".
	* @param[in] tz Part olson time zone, for example to list all time zones in Europe,
	* set it to: "Europe" and it will returns: "Europe/Amsterdam","Europe/Andorra","Europe/Astrakhan", ...
	* If set to NULL, all supported time zones are retrieved.
	* @return Returns HTTP code of result.
	*/
	WorldTimeAPI_HttpCode getListOfTimeZones(std::string& list, const char* tz = NULL);
#endif // ARDUINO

	/**
	* @brief Gets time zone informations by time zone name.
	* @param[in] tz Olson time zone name, for example: "Europe/Amsterdam".
	* If only partial time zone is specified (for example: "Europe"), http code will be set to
	* WTA_ERROR_PARTIAL_TIMEZONE, because API returned list of accepted time zones instead of time
	* zone info. To get that list, call getListOfTimeZones();
	* @return Returns constant reference to result.
	*/
	const WorldTimeAPIResult& getByTimeZone(const char* tz);

	/**
	* @brief Gets time zone informations by public IP address.
	* @param[in] IP Text with valid IPv4 address. If set to null, current public IP address is used.
	* @note Only IPv4 addresses are supported.
	* @return Returns constant reference to result.
	*/
	const WorldTimeAPIResult& getByIP(const char* IP = NULL);

#if (defined(ESP8266) || defined(ESP32)) && defined(ARDUINO)
	/**
	* @brief Gets time zone informations by public IP address.
	* @param[in] IP Valid IPv4 address.
	* @note Only IPv4 addresses are supported.
	* @return Returns constant reference to result.
	*/
	const WorldTimeAPIResult& getByIP(const IPAddress& IP);
#endif // !SJSONP_UNDER_OS

	/**
	* @brief Gets last result from getByIP() or getByTimeZone() method.
	*/
	inline const WorldTimeAPIResult& getLastResult() const{
		return lastRes;
	}

protected:

	/**
	* @brief Last result from getByIP() or getByTimeZone() method.
	*/
	WorldTimeAPIResult lastRes;

	const WorldTimeAPIResult& getAndParseTZ(const char* url);

	static bool jsonItemTZ(JSONItemType type, const char* key, int keyLength, const SimpleJSONTextParser::Number& parsedVal, int depth, int index, void* owner_ptr);

	static bool jsonTextTZ(const char* key, int keyLength, const char* value, int valueLength, int depth, int index, void* owner_ptr);

	static bool jsonControlTZ(JSONItemType type, const char* key, int keyLength, int depth, int index, void* owner_ptr);

	static bool jsonItemERR(JSONItemType type, const char* key, int keyLength, const SimpleJSONTextParser::Number& parsedVal, int depth, int index, void* owner_ptr);

	static bool jsonTextERR(const char* key, int keyLength, const char* value, int valueLength, int depth, int index, void* owner_ptr);


	static const char* URL_TimeZone;
	static const char* URL_IP;

#if defined(SJSONP_UNDER_OS)
	static WorldTimeAPI_HttpCode requestGET(const char* url, std::string& resp);
#elif (defined(ESP8266) || defined(ESP32)) && defined(ARDUINO)
	static WorldTimeAPI_HttpCode requestGET(const char* url, String& resp);
#endif // !SJSONP_UNDER_OS


#if defined(SJSONP_UNDER_OS)
	/**
	* @brief Calls command (CMD) and retrieves it's result.
	* @param command Command to call.
	* @return Returns response to command.
	*/
	static std::string ssystem(const char* command);
#endif // !SJSONP_UNDER_OS



	class WorldTimeAPIResHelper {
	public:
		WorldTimeAPIResHelper(WorldTimeAPIResult* result_ptr_) : result_ptr(result_ptr_)
		{}

		/**
		* @struct FFlags
		* @brief Structure, which represents found flags.
		*/
		struct FFlags {

			FFlags() :
				abbreviation_found(false),
				client_ip_found(false),
				dst_found(false),
				dst_from_found(false),
				dst_offset_found(false),
				dst_until_found(false),
				raw_offset_found(false),
				timezone_found(false),
				unixtime_found(false),
				error_found(false)
			{}

			FFlags(uint16_t raw) {
				const FFlags* tmp = reinterpret_cast<const FFlags*>(&raw);
				*this = *tmp;
			}

			operator uint16_t& () {
				return *(reinterpret_cast<uint16_t*>(this));
			}

			operator uint16_t() const {
				return *(reinterpret_cast<const uint16_t*>(this));
			}

			bool abbreviation_found : 1;
			bool client_ip_found : 1;
			bool dst_found : 1;
			bool dst_from_found : 1;
			bool dst_offset_found : 1;
			bool dst_until_found : 1;
			bool raw_offset_found : 1;
			bool timezone_found : 1;

			bool unixtime_found : 1;
			bool error_found : 1;

			inline bool allValidFound() const {
				return *(reinterpret_cast<const uint16_t*>(this)) >= 0x1FF && !error_found;
			}
		};

		/**
		* @brief Found keys flags.
		*/
		FFlags foundFlags;

		/**
		* @brief Pointer to WorldTimeAPIResult.
		*/
		WorldTimeAPIResult* result_ptr;

		bool dst = false;
		bool dst_null = false;

		TimeZone tz;
		DateTime dst_from;
		DateTime dst_until;
		int16_t dst_offset = 0;

		DateTimeSysSync unixtime;
	};
};


#endif // !WORLD_TIME_API_H_

