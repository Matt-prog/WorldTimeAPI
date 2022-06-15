/**
 * @file SimpleJSONParser.h
 * @brief This file contains simple JSON parser class.
 *
 * @see SimpleJSONParser
 *
 * # Credits
 * @author Matej Fitoš
 * @date Jun 12, 2022
 */


#ifndef SIMPLE_JSON_PARSER_H_
#define SIMPLE_JSON_PARSER_H_

#ifndef ARDUINO
#include <cstdint>
#else
#include <Arduino.h>
#endif

#if defined(_WIN64) || defined(_WIN32) || defined(__APPLE__) || defined(__linux) || defined(__unix) || defined(__posix)
#include <iostream>
#define SJSONP_UNDER_OS (1)
#endif


typedef enum {
	JIT_String,
	JIT_Number,
	JIT_Bool,
	JIT_Null,
	JIT_ObjectBegin,
	JIT_ObjectEnd,
	JIT_ArrayBegin,
	JIT_ArrayEnd
}JSONItemType;


/**
* @class SimpleJSONParser
* @brief This class makes parsing of JSON easier. Parsing is done using 
* <a href="https://en.wikipedia.org/wiki/Simple_API_for_XML">SAX</a>.
* So callback events are called during parsing. There are 3 callback
* functions, that are called:
* + **onTextItemFound** - this function is called, when text item is found, for example: "key":"value".
* + **onItemFound** - this function is called, when number, *true*, *false* or *null* is found in JSON.
* + **onObjArrFound** - this function is called, when object begin/end or array begin/end is found.
* All those functions has to be set before parsing.
*/
class SimpleJSONParser {
public:

	/**
	* @struct Number
	* @brief Structure, which can contain any number.
	*/
	struct Number {

		typedef enum {
			NT_Null,
			NT_Int64Ovf, //Overflow
			NT_Uint64Ovf, //Overflow
			NT_Bool,
			NT_Int8,
			NT_Uint8,
			NT_Int16,
			NT_Uint16,
			NT_Int32,
			NT_Uint32,
			NT_Int64,
			NT_Uint64,
			NT_Float,
			NT_Double
		}NumberType;

		union NumberUnion {
			double Double;
			float Float;
			int64_t Int64;
			uint64_t Uint64;
			int32_t Int32;
			uint32_t Uint32;
			int16_t Int16;
			uint16_t Uint16;
			int8_t Int8;
			uint8_t Uint8;
			bool Bool;
		};

		/**
		* @brief Number type.
		*/
		NumberType Type;

		/**
		* @brief Actual number value.
		*/
		NumberUnion Value;

		static const Number Null;

		Number(double val) : Type(NT_Double) {
			Value.Double = val;
		}

		Number(float val) : Type(NT_Float) {
			Value.Float = val;
		}

		Number(int64_t val, bool ovf = false) {
			Type = (ovf) ? NT_Int64Ovf : NT_Int64;
			Value.Int64 = val;
		}

		Number(uint64_t val, bool ovf = false) {
			Type = (ovf) ? NT_Uint64Ovf : NT_Uint64;
			Value.Uint64 = val;
		}

		Number(int32_t val) : Type(NT_Int32) {
			Value.Int32 = val;
		}

		Number(uint32_t val) : Type(NT_Uint32) {
			Value.Uint32 = val;
		}

		Number(int16_t val) : Type(NT_Int16) {
			Value.Int16 = val;
		}

		Number(uint16_t val) : Type(NT_Uint16) {
			Value.Uint16 = val;
		}

		Number(int8_t val) : Type(NT_Int8) {
			Value.Int8 = val;
		}

		Number(uint8_t val) : Type(NT_Uint8) {
			Value.Uint8 = val;
		}

		Number(bool val) : Type(NT_Bool) {
			Value.Uint64 = val;
		}

		Number() : Type(NT_Null) {
			Value.Uint64 = 0;
		}

#ifdef SJSONP_UNDER_OS
		friend std::ostream& operator<<(std::ostream& os, const SimpleJSONParser::Number& m);
#endif // !SJSONP_UNDER_OS

		/**
		* @brief Parses any number from text. Parsing stops, when non numeric character is found.
		* Example of values, that can be parsed:
		* @verbatim
		* 1 -> uint16_t 1
		* -1 -> int16_t -1
		* 12345678910 -> uint64_t 12345678910
		* 1.128 -> double 1.128
		* 1.2e5 -> double 1.2e+5
		* 1.2e+5 -> double 1.2e+5
		* 1.2e-5 -> double 1.2e-5
		* 1.2E5 -> double 1.2e+5
		* @endverbatim
		* @note Only decimal system (DEC) valeus can be parsed.
		* @param text Text to be parsed.
		* @param textSize Size of text buffer or length of text to be parsed.
		* @return Returns negative position of character, which causes parsing error. Or
		* return positive value of position of character after last parsed character, when
		* parsing was successfull.
		* @todo Parsed double value can be inaccurate - fix it.
		*/
		int parse(const char* text, int textSize);
	};

	/**
	* @brief Parses string with JSON.
	* ## Parsing principle
	* There are three callback functions, that has to be specified before calling this function:
	* + **onTextItemFound** - this function is called, when text item is found, for example: "key":"value".
	* + **onItemFound** - this function is called, when number, *true*, *false* or *null* is found in JSON.
	* + **onObjArrFound** - this function is called, when object begin/end or array begin/end is found.
	* Each time, array or object begin is found, *depth* parameter is incremented. Value of depth at the beginning
	* is 0. See this example:
	* @verbatim
	* {                         <- Depth = 0
	*   "key":"value",          <- Depth = 1
	*   "object":{
	*     "key":"value",        <- Depth = 2
	*     "array":[
	*       "text1",            <- Depth = 3
	*       "text2"
	*     ],
	*     "key2":"value"        <- Depth = 2
	*   },
	*   "key2":"value"          <- Depth = 1
	* }
	* @endverbatim
	* Another significant parameter is *index*, which specifies index of item in object or in array. See this example:
	* @verbatim
	* {
	*   "key":"value",          <- Index = 0
	*   "object":{              <- Index = 1
	*     "key":"value",        <- Index = 0
	*     "array":[             <- Index = 1
	*       "text1",            <- Index = 0
	*       "text2"             <- Index = 1
	*     ],
	*     "key2":"value"        <- Index = 2
	*   },
	*   "key2":"value"          <- Index = 2
	* }
	* @enverbatim
	* There are two text parameters (*key* and *value*) in this functions, which has to be handled in differrent way.
	* Those values are not null terminated, because they points directly to json string. To know it's length, *keyLength*
	* and *valueLength* need to be read. So basic C functions, which works with string with null termination cannot be used
	* (for example strcmp or strcpy). It is recommended to use std::string or String on arduino like this:
	* @code{.cpp}
	* //Code for PC
	* std::string strVal = std::string(value, valueLength); //Creates copy of value from string of given length
	* 
	* //Code for Arduino
	* String strVal = String(value, valueLength); //Creates copy of value from string of given length
	* @endcode
	* This code has one problem. String will contain escaped characters, which are not converted to normal characters.
	* To escape characters use unescape() function:
	* @code{.cpp}
	* //Code for PC
	* std::string strVal = SimpleJSONParser::unescape(value, valueLength); //Creates copy of value from string of given length
	* 
	* //Code for Arduino
	* String strVal = SimpleJSONParser::unescape(value, valueLength); //Creates copy of value from string of given length
	* @endcode
	* 
	* All three callback function has to return true, if parsing can continue or false to stop parsing due to error.
	* 
	* @param[in] json String with JSON to parse. Leading white space is ignored.
	* @param[in] jsonSize Size of buffer (including null terminator), where string with JSON is located.
	* @param[in] owner_ptr Pointer to owner instance. This value is not changed, readed nod member function is executed,
	* it is only for user, to identify which instance of owner class called this function.
	* @return Returns positive non zero value, when parsing was successful. This value is index of character after last parsed character.
	* Returns negative or zero value, if parsing failed. Absolute value of returned value in this case is position of character,
	* where parsing failed.
	*/
	int parseJSON(const char* json, int jsonSize, void* owner_ptr = NULL);

	bool (*onTextItemFound)(const char* key, int keyLength, const char* value, int valueLength, int depth, int index, void* owner_ptr) = nullptr;
	bool (*onItemFound)(JSONItemType type, const char* key, int keyLength, const Number& parsedVal, int depth, int index, void* owner_ptr) = nullptr;
	bool (*onObjArrFound)(JSONItemType type, const char* key, int keyLength, int depth, int index, void* owner_ptr) = nullptr;

	/**
	* @brief Unescapes escaped string from JSON and copies it to another buffer.
	* @param[out] destStr Buffer, where characters will be copied.
	* @param[out] destSize Size of destination buffer including null terminator.
	* @param[in] sourceStr Source escaped text from JSON.
	* @param[in] sourceSize Count of characters.
	* @return Returns unescaped string.
	* @todo Unicode characters are not uescaped right now.
	*/
	static void unescapeAndCopy(char* destStr, int destSize, const char* sourceStr, int sourceSize);

#ifdef ARDUINO
	/**
	* @brief Unescapes escaped string from JSON.
	* @param[in] sourceStr Source escaped text from JSON.
	* @param[in] sourceSize Count of characters.
	* @return Returns unescaped string.
	* @todo Unicode characters are not uescaped right now.
	*/
	static String unescape(const char* sourceStr, int sourceSize);
#else
	/**
	* @brief Unescapes escaped string from JSON.
	* @param[in] sourceStr Source escaped text from JSON.
	* @param[in] sourceSize Count of characters.
	* @return Returns unescaped string.
	* @todo Unicode characters are not uescaped right now.
	*/
	static std::string unescape(const char* sourceStr, int sourceSize);
#endif // ARDUINO

protected:
	/*int parseObject(const char* json, int jsonSize, int depth, void* owner_ptr);
	int parseArray(const char* json, int jsonSize, const char* key, int keyLength, int depth, void* owner_ptr);*/

	int parseObjArr(const char* json, int jsonSize, bool isObject, const char* key, int keyLength, int depth, void* owner_ptr);

	static const char* null;
	static const char* True;
	static const char* False;
};


#endif // !SIMPLE_JSON_PARSER_H_
