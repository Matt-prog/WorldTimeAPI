#include "SimpleJSONParser.h"

const SimpleJSONParser::Number SimpleJSONParser::Number::Null = SimpleJSONParser::Number();

#ifdef SJSONP_UNDER_OS
std::ostream& operator<<(std::ostream& os, const SimpleJSONParser::Number& m)
{
	switch (m.Type) {
		case SimpleJSONParser::Number::NT_Null: os << "null"; break;
		case SimpleJSONParser::Number::NT_Double: os << m.Value.Double; break;
		case SimpleJSONParser::Number::NT_Float: os << m.Value.Float; break;
		case SimpleJSONParser::Number::NT_Int64Ovf:
		case SimpleJSONParser::Number::NT_Int64: os << m.Value.Int64; break;
		case SimpleJSONParser::Number::NT_Uint64Ovf:
		case SimpleJSONParser::Number::NT_Uint64: os << m.Value.Uint64; break;
		case SimpleJSONParser::Number::NT_Int32: os << m.Value.Int32; break;
		case SimpleJSONParser::Number::NT_Uint32: os << m.Value.Uint32; break;
		case SimpleJSONParser::Number::NT_Int16: os << m.Value.Int16; break;
		case SimpleJSONParser::Number::NT_Uint16: os << m.Value.Uint16; break;
		case SimpleJSONParser::Number::NT_Int8: os << m.Value.Int8; break;
		case SimpleJSONParser::Number::NT_Uint8: os << m.Value.Uint8; break;
		case SimpleJSONParser::Number::NT_Bool: os << m.Value.Bool; break;
		default: os << "unknown"; break;
	}
	return os;
}
#endif // !SJSONP_UNDER_OS

int SimpleJSONParser::Number::parse(const char* json, int jsonSize) {
	Type = SimpleJSONParser::Number::NT_Null;
	if (jsonSize == 0) return 0;
	uint64_t value = 0;
	bool ovf = false;
	double dvalue = 0.00;
	double fract = 0.1;
	int8_t exponent = 0;
	bool isNegExp = false;
	uint8_t exponentDigitsCount = 0;

	bool isReal = false;
	bool dotFound = false;
	bool expFound = false;

	bool isNegative = json[0] == '-';
	int i = 0;
	if (json[0] == '+' || isNegative) { //Skip sign
		i++;
	}

	for (; i < jsonSize && json[i] != 0; i++) {
		char c = json[i];
		if (c >= '0' && c <= '9') {
			if (expFound) {
				if (exponentDigitsCount > 1) {
					//ERROR: exponent contains 3 or more digits
					return -i;
				}
				exponent *= 10;
				exponent += c - '0';
				exponentDigitsCount++;
			}
			else if (isReal) {
				//Real
				if (dotFound) {
					double v = c - '0';
					dvalue += v * fract;
					fract /= 10;
				}
				else {
					dvalue *= 10.00;
					dvalue += c - '0';
				}
			}
			else {
				//Integer
				uint64_t newVal = value;
				newVal *= 10;
				newVal += c - '0';
				if (newVal < value) {
					ovf = true;
				}
				value = newVal;
			}
		}
		else if (c == '.') {
			if (dotFound || expFound) {
				return -i; //Error, dot was found 2 times or in exponent
			}
			else {
				dotFound = true;
				if (!isReal) {
					dvalue = value;
				}
			}
			isReal = true;
		}
		else if (c == 'e' || c == 'E') {
			if (i + 1 < jsonSize) {
				char c2 = json[i + 1];
				bool nextIsNum = c2 >= '0' && c2 <= '9';
				isNegExp = c2 == '-';
				bool nextIsSign = isNegExp || c2 == '+';
				if (!(nextIsNum || nextIsSign)) {
					return -i; //Error, expected number after exponent
				}
				if (nextIsSign) {
					i++; //Skip sign
				}
			}
			else {
				return -i; //Error, expected number after exponent
			}

			if (expFound) {
				return -i; //Error, exponent found two times
			}
			else {
				expFound = true;
				if (!isReal) {
					dvalue = value;
				}
			}
			isReal = true;
		}
		else {
			break; //Parsing stops
		}
	}

	if (isReal) {
		//Real number
		if (isNegative) {
			dvalue = -dvalue;
		}

		if (expFound && exponent != 0) {
			if (isNegExp) {
				exponent = -exponent;
			}

			//Appling exponent
			if (exponent > 0) {
				//Positive exponent
				while (exponent > 0) {
					dvalue *= 10.00;
					exponent--;
				}
			}
			else {
				//Negative exponent
				while (exponent < 0) {
					dvalue /= 10.00;
					exponent++;
				}
			}
		}
		Type = SimpleJSONParser::Number::NT_Double;
		Value.Double = dvalue;
	}
	else {
		//Integer
		if (isNegative) {
			int64_t negVal = -(int64_t)value;
			if (ovf || negVal >= 0 || negVal < INT64_MIN) {
				Type = SimpleJSONParser::Number::NT_Int64Ovf;
			}
			else if (negVal < INT32_MIN) {
				Type = SimpleJSONParser::Number::NT_Int64;
			}
			else if (negVal < INT16_MIN) {
				Type = SimpleJSONParser::Number::NT_Int32;
			}
			else {
				Type = SimpleJSONParser::Number::NT_Int16;
			}
			Value.Int64 = negVal;
		}
		else {
			if (ovf) {
				Type = SimpleJSONParser::Number::NT_Uint64Ovf;
			}
			else if (value > UINT32_MAX) {
				Type = SimpleJSONParser::Number::NT_Uint64;
			}
			else if (value > UINT16_MAX) {
				Type = SimpleJSONParser::Number::NT_Uint32;
			}
			else {
				Type = SimpleJSONParser::Number::NT_Uint16;
			}
			Value.Uint64 = value;
		}
	}

	return i;
}


int SimpleJSONParser::parseJSON(const char* json, int jsonSize, void* owner_ptr) {
	int i = 0;
	for (; i < jsonSize && json[i] <= ' '; i++); //Skipping white characters
	
	if (json[i] == '{') {
		i++;

		if (onObjArrFound != NULL) {
			if (!onObjArrFound(JSONItemType::JIT_ObjectBegin, "", 0, 0, 0, owner_ptr)) {
				return -i; //ERROR: user error
			}
		}
		int pRes = parseObjArr(json + i, jsonSize - i, true, "", 0, 1, owner_ptr);
		if (pRes <= 0) {
			//Parsing ERROR
			return pRes - i;
		}

		if (onObjArrFound != NULL) {
			if (!onObjArrFound(JSONItemType::JIT_ObjectEnd, "", 0, 0, 0, owner_ptr)) {
				return -i; //ERROR: user error
			}
		}
		
		return pRes + i;
	}
	else if (json[i] == '[') {
		i++;

		if (onObjArrFound != NULL) {
			if (!onObjArrFound(JSONItemType::JIT_ArrayBegin, "", 0, 0, 0, owner_ptr)) {
				return -i; //ERROR: user error
			}
		}
		int pRes = parseObjArr(json + i, jsonSize - i, false, "", 0, 1, owner_ptr);
		if (pRes <= 0) {
			//Parsing ERROR
			return pRes - i;
		}

		if (onObjArrFound != NULL) {
				if (!onObjArrFound(JSONItemType::JIT_ArrayEnd, "", 0, 0, 0, owner_ptr)) {
					return -i; //ERROR: user error
				}
		}

		return pRes + i;
	}
	else {
		//Start or json expected
		return -i;
	}
}

/*
int SimpleJSONParser::parseObject(const char* json, int jsonSize, int depth, void* owner_ptr) {
	uint8_t step = 0;
	//1 - begin of key found
	//2 - end of key found
	//3 - key - value separator found
	//4 - begin of value found
	//5 - end of value found

	int index = 0;
	const char* key = NULL;
	int keyLength = 0;
	const char* value = NULL;
	int valueLength = 0;
	JSONItemType valueType;

	bool isEscaped = false;

	int i = 0;
	for (; i < jsonSize && json[i] != 0; i++) {
		char c = json[i]; //Real character
		char ec = c; //Escaped character
		bool isSepQuot = !isEscaped && c == '"';

		//Escaping characters
		if (isEscaped) {
			switch (c) {
			case 'b': ec = '\b'; break;
			case 'f': ec = '\f'; break;
			case 'n': ec = '\n'; break;
			case 'r': ec = '\r'; break;
			case 't': ec = '\t'; break;
			}
			isEscaped = false;
		}
		else if (c == '\\') {
			isEscaped = true;
			//continue;
		}

		if (isSepQuot) {
			if (step >= 0 && step <= 4 && step != 2) {
				switch (step) {
				case 0: key = json + i + 1; keyLength = 0; break;
				case 3: value = json + i + 1; valueLength = 0; valueType = JSONItemType::JIT_String; break;
				case 4:
					if (valueType != JSONItemType::JIT_String) {
						//ERROR: Text end found, but value type was not text
						return -i;
					}
					if (onTextItemFound != NULL) {
						onTextItemFound(key, keyLength, value, valueLength, depth, index, owner_ptr);
					}
					break;
				}
				step++;
			}
			else {
				//ERROR: unexpected '"' found
				return -i;
			}
		}
		else {
			bool inText = step == 1 || (step == 4 && valueType == JSONItemType::JIT_String);
			bool isDigit = c >= '0' && c <= '9';
			bool isNumeric = isDigit || c == '+' || c == '-' || c == '.';

			if (inText) {
				//Text character found
				if (step == 1) {
					keyLength++;
				}
				else {
					valueLength++;
				}
			}
			else if (step == 3 && (c == null[0] || c == True[0] || c == False[0] || isNumeric)) {
				//In non text value (e.g. null, true, false or number)
				valueType = JSONItemType::JIT_Number;
				if (isNumeric) {
					Number parsVal;
					int pRes = parsVal.parse(json + i, jsonSize - i);
					if (pRes <= 0 || i + pRes >= jsonSize) {
						//Parsing ERROR
						return pRes - i;
					}
					char cap = json[i + pRes];
					if (cap != ' ' && cap != '\t' && cap != '\r' && cap != '\n' && cap != ',' && cap != '}' && cap != ']') {
						//Parsing ERROR - parsing stopped at character, that was not valid
						return pRes - i;
					}
					if (onItemFound != NULL) {
						onItemFound(valueType, key, keyLength, parsVal, depth, index, owner_ptr);
					}
					i += pRes - 1;
				}
				else {
					//null, true or false
					int j = 1;
					if (c == null[0]) {
						valueType = JSONItemType::JIT_Null;
						for (; i + j < jsonSize && json[i + j] == null[j] && json[i + j] != '\0'; j++);
						if (null[j] != '\0' || i + j >= jsonSize || json[i + j] == '\0') {
							//ERROR: Text end found, but value type was not text
							return -(i + j);
						}
						if (onItemFound != NULL) {
							onItemFound(valueType, key, keyLength, Number::Null, depth, index, owner_ptr);
						}
					}
					else if (c == True[0]) {
						valueType = JSONItemType::JIT_Bool;
						for (; i + j < jsonSize && json[i + j] == True[j] && json[i + j] != '\0'; j++);
						if (True[j] != '\0' || i + j >= jsonSize || json[i + j] == '\0') {
							//ERROR: Text end found, but value type was not text
							return -(i + j);
						}
						if (onItemFound != NULL) {
							onItemFound(valueType, key, keyLength, Number(true), depth, index, owner_ptr);
						}
					}
					else {
						valueType = JSONItemType::JIT_Bool;
						for (; i + j < jsonSize && json[i + j] == False[j] && json[i + j] != '\0'; j++);
						if (False[j] != '\0' || i + j >= jsonSize || json[i + j] == '\0') {
							//ERROR: Text end found, but value type was not text
							return -(i + j);
						}
						if (onItemFound != NULL) {
							onItemFound(valueType, key, keyLength, Number(false), depth, index, owner_ptr);
						}
					}
					i = i + j - 1;
				}
				step = 5; //End of value found
			}
			else {
				//Characters outside of text
				bool isKeyValSep = step == 2 && c == ':';
				bool isItemSep = step == 5 && c == ',';

				//bool isObjectStart = (step == 0 || step == 3) && c == '{';
				bool isObjectStart = step == 3 && c == '{';
				bool isObjectEnd = (step == 0 || step == 5) && c == '}';
				bool isArrayStart = step == 3 && c == '[';

				if (isKeyValSep) {
					step++;
				}
				else if (isItemSep) {
					step = 0;
					index++;
				}
				else if (isObjectStart) {
					valueType = JSONItemType::JIT_ObjectBegin;
					if (onObjArrFound != NULL) {
						onObjArrFound(valueType, key, keyLength, depth, index, owner_ptr);
					}
					int pRes = parseObject(json + i + 1, jsonSize - i - 1, depth + 1, owner_ptr);
					if (pRes <= 0) {
						//Parsing ERROR
						return pRes - i;
					}
					valueType = JSONItemType::JIT_ObjectEnd;
					if (onObjArrFound != NULL) {
						onObjArrFound(valueType, key, keyLength, depth, index, owner_ptr);
					}
					i += pRes;
					step = 5; //End of value found
				}
				else if (isObjectEnd) {
					return i + 1; //Object is finally parsed
				}
				else if (isArrayStart) {
					valueType = JSONItemType::JIT_ArrayBegin;
					if (onObjArrFound != NULL) {
						onObjArrFound(valueType, key, keyLength, depth, index, owner_ptr);
					}
					int pRes = parseArray(json + i + 1, jsonSize - i - 1, key, keyLength, depth + 1, owner_ptr);
					if (pRes <= 0) {
						//Parsing ERROR
						return pRes - i;
					}
					valueType = JSONItemType::JIT_ArrayEnd;
					if (onObjArrFound != NULL) {
						onObjArrFound(valueType, key, keyLength, depth, index, owner_ptr);
					}
					i += pRes;
					step = 5; //End of value found
				}
				else if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
					//ERROR: unexpected character found outside of text
					return -i;
				}
			}
		}
	}
	return -i; //Parsing failed
}


int SimpleJSONParser::parseArray(const char* json, int jsonSize, const char* key, int keyLength, int depth, void* owner_ptr) {
	uint8_t step = 0;
	//1 - begin of value found
	//2 - end of value found

	int index = 0;
	const char* value = NULL;
	int valueLength = 0;
	JSONItemType valueType;

	bool isEscaped = false;

	int i = 0;
	for (; i < jsonSize && json[i] != 0; i++) {
		char c = json[i]; //Real character
		char ec = c; //Escaped character
		bool isSepQuot = !isEscaped && c == '"';

		//Escaping characters
		if (isEscaped) {
			switch (c) {
			case 'b': ec = '\b'; break;
			case 'f': ec = '\f'; break;
			case 'n': ec = '\n'; break;
			case 'r': ec = '\r'; break;
			case 't': ec = '\t'; break;
			}
			isEscaped = false;
		}
		else if (c == '\\') {
			isEscaped = true;
			//continue;
		}

		if (isSepQuot) {
			if (step >= 0 && step <= 1) {
				switch (step) {
				case 0: value = json + i + 1; valueLength = 0; valueType = JSONItemType::JIT_String; break;
				case 1:
					if (valueType != JSONItemType::JIT_String) {
						//ERROR: Text end found, but value type was not text
						return -i;
					}
					if (onTextItemFound != NULL) {
						onTextItemFound(key, keyLength, value, valueLength, depth, index, owner_ptr);
					}
					break;
				}
				step++;
			}
			else {
				//ERROR: unexpected '"' found
				return -i;
			}
		}
		else {
			bool inText = (step == 1 && valueType == JSONItemType::JIT_String);
			bool isDigit = c >= '0' && c <= '9';
			bool isNumeric = isDigit || c == '+' || c == '-' || c == '.';

			if (inText) {
				//Text character found
				valueLength++;
			}
			else if (step == 0 && (c == null[0] || c == True[0] || c == False[0] || isNumeric)) {
				//In non text value (e.g. null, true, false or number)
				valueType = JSONItemType::JIT_Number;
				if (isNumeric) {
					Number parsVal;
					int pRes = parsVal.parse(json + i, jsonSize - i);
					if (pRes <= 0 || i + pRes >= jsonSize) {
						//Parsing ERROR
						return pRes - i;
					}
					char cap = json[i + pRes];
					if (cap != ' ' && cap != '\t' && cap != '\r' && cap != '\n' && cap != ',' && cap != '}' && cap != ']') {
						//Parsing ERROR - parsing stopped at character, that was not valid
						return pRes - i;
					}
					if (onItemFound != NULL) {
						onItemFound(valueType, key, keyLength, parsVal, depth, index, owner_ptr);
					}
					i += pRes - 1;
				}
				else {
					//null, true or false
					int j = 1;
					if (c == null[0]) {
						valueType = JSONItemType::JIT_Null;
						for (; i + j < jsonSize && json[i + j] == null[j] && json[i + j] != '\0'; j++);
						if (null[j] != '\0' || i + j >= jsonSize || json[i + j] == '\0') {
							//ERROR: Text end found, but value type was not text
							return -(i + j);
						}
						if (onItemFound != NULL) {
							onItemFound(valueType, key, keyLength, Number::Null, depth, index, owner_ptr);
						}
					}
					else if (c == True[0]) {
						valueType = JSONItemType::JIT_Bool;
						for (; i + j < jsonSize && json[i + j] == True[j] && json[i + j] != '\0'; j++);
						if (True[j] != '\0' || i + j >= jsonSize || json[i + j] == '\0') {
							//ERROR: Text end found, but value type was not text
							return -(i + j);
						}
						if (onItemFound != NULL) {
							onItemFound(valueType, key, keyLength, Number(true), depth, index, owner_ptr);
						}
					}
					else {
						valueType = JSONItemType::JIT_Bool;
						for (; i + j < jsonSize && json[i + j] == False[j] && json[i + j] != '\0'; j++);
						if (False[j] != '\0' || i + j >= jsonSize || json[i + j] == '\0') {
							//ERROR: Text end found, but value type was not text
							return -(i + j);
						}
						if (onItemFound != NULL) {
							onItemFound(valueType, key, keyLength, Number(false), depth, index, owner_ptr);
						}
					}
					i = i + j - 1;
				}
				step = 2; //End of value found
			}
			else {
				//Characters outside of text
				bool isItemSep = step == 2 && c == ',';

				bool isObjectStart = step == 0 && c == '{';
				bool isArrayStart = step == 0 && c == '[';
				bool isArrayEnd = (step == 0 || step == 2) && c == ']';

				if (isItemSep) {
					step = 0;
					index++;
				}
				else if (isObjectStart) {
					valueType = JSONItemType::JIT_ObjectBegin;
					if (onObjArrFound != NULL) {
						onObjArrFound(valueType, key, keyLength, depth, index, owner_ptr);
					}
					int pRes = parseObject(json + i + 1, jsonSize - i - 1, depth + 1, owner_ptr);
					if (pRes <= 0) {
						//Parsing ERROR
						return pRes - i;
					}
					valueType = JSONItemType::JIT_ObjectEnd;
					if (onObjArrFound != NULL) {
						onObjArrFound(valueType, key, keyLength, depth, index, owner_ptr);
					}
					i += pRes;
					step = 2; //End of value found
				}
				else if (isArrayStart) {
					valueType = JSONItemType::JIT_ArrayBegin;
					if (onObjArrFound != NULL) {
						onObjArrFound(valueType, key, keyLength, depth, index, owner_ptr);
					}
					int pRes = parseArray(json + i + 1, jsonSize - i - 1, key, keyLength, depth + 1, owner_ptr);
					if (pRes <= 0) {
						//Parsing ERROR
						return pRes - i;
					}
					valueType = JSONItemType::JIT_ArrayEnd;
					if (onObjArrFound != NULL) {
						onObjArrFound(valueType, key, keyLength, depth, index, owner_ptr);
					}
					i += pRes;
					step = 2; //End of value found
				}
				else if (isArrayEnd) {
					return i + 1; //Aray is finally parsed
				}
				else if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
					//ERROR: unexpected character found outside of text
					return -i;
				}
			}
		}
	}
	return -i; //Parsing failed
}

*/

int SimpleJSONParser::parseObjArr(const char* json, int jsonSize, bool isObject, const char* key, int keyLength, int depth, void* owner_ptr) {
	uint8_t step;
	//1 - begin of key found           - Not used in arrays
	//2 - end of key found             - Not used in arrays
	//3 - key - value separator found  - On arrays, this is used instead of 0
	//4 - begin of value found
	//5 - end of value found

	int index = 0;
	const char* value = NULL;
	int valueLength = 0;
	JSONItemType valueType;

	bool isEscaped = false;

	if (isObject) {
		key = NULL;
		keyLength = 0;
		step = 0;
	}
	else {
		step = 3;
	}

	int i = 0;
	for (; i < jsonSize && json[i] != 0; i++) {
		char c = json[i]; //Real character
		char ec = c; //Escaped character
		bool isSepQuot = !isEscaped && c == '"';

		//Escaping characters
		if (isEscaped) {
			switch (c) {
			case 'b': ec = '\b'; break;
			case 'f': ec = '\f'; break;
			case 'n': ec = '\n'; break;
			case 'r': ec = '\r'; break;
			case 't': ec = '\t'; break;
			}
			isEscaped = false;
		}
		else if (c == '\\') {
			isEscaped = true;
			//continue;
		}

		if (isSepQuot) {
			if (step >= 0 && step <= 4 && step != 2) {
				switch (step) {
				case 0: key = json + i + 1; keyLength = 0; break;
				case 3: value = json + i + 1; valueLength = 0; valueType = JSONItemType::JIT_String; break;
				case 4:
					if (valueType != JSONItemType::JIT_String) {
						//ERROR: Text end found, but value type was not text
						return -i;
					}
					if (onTextItemFound != NULL) {
						if (!onTextItemFound(key, keyLength, value, valueLength, depth, index, owner_ptr)) {
							return -i; //ERROR: user error
						}
					}
					break;
				}
				step++;
			}
			else {
				//ERROR: unexpected '"' found
				return -i;
			}
		}
		else {
			bool inText = step == 1 || (step == 4 && valueType == JSONItemType::JIT_String);
			bool isDigit = c >= '0' && c <= '9';
			bool isNumeric = isDigit || c == '+' || c == '-' || c == '.';

			if (inText) {
				//Text character found
				if (step == 1) {
					keyLength++;
				}
				else {
					valueLength++;
				}
			}
			else if (step == 3 && (c == null[0] || c == True[0] || c == False[0] || isNumeric)) {
				//In non text value (e.g. null, true, false or number)
				valueType = JSONItemType::JIT_Number;
				if (isNumeric) {
					Number parsVal;
					int pRes = parsVal.parse(json + i, jsonSize - i);
					if (pRes <= 0 || i + pRes >= jsonSize) {
						//Parsing ERROR
						return pRes - i;
					}
					char cap = json[i + pRes];
					if (cap != ' ' && cap != '\t' && cap != '\r' && cap != '\n' && cap != ',' && cap != '}' && cap != ']') {
						//Parsing ERROR - parsing stopped at character, that was not valid
						return pRes - i;
					}
					if (onItemFound != NULL) {
						if (!onItemFound(valueType, key, keyLength, parsVal, depth, index, owner_ptr)) {
							return -i; //ERROR: user error
						}
					}
					i += pRes - 1;
				}
				else {
					//null, true or false
					int j = 1;
					if (c == null[0]) {
						valueType = JSONItemType::JIT_Null;
						for (; i + j < jsonSize && json[i + j] == null[j] && json[i + j] != '\0'; j++);
						if (null[j] != '\0' || i + j >= jsonSize || json[i + j] == '\0') {
							//ERROR: Text end found, but value type was not text
							return -(i + j);
						}
						if (onItemFound != NULL) {
							if (!onItemFound(valueType, key, keyLength, Number::Null, depth, index, owner_ptr)) {
								return -i; //ERROR: user error
							}
						}
					}
					else if (c == True[0]) {
						valueType = JSONItemType::JIT_Bool;
						for (; i + j < jsonSize && json[i + j] == True[j] && json[i + j] != '\0'; j++);
						if (True[j] != '\0' || i + j >= jsonSize || json[i + j] == '\0') {
							//ERROR: Text end found, but value type was not text
							return -(i + j);
						}
						if (onItemFound != NULL) {
							if (!onItemFound(valueType, key, keyLength, Number(true), depth, index, owner_ptr)) {
								return -i; //ERROR: user error
							}
						}
					}
					else {
						valueType = JSONItemType::JIT_Bool;
						for (; i + j < jsonSize && json[i + j] == False[j] && json[i + j] != '\0'; j++);
						if (False[j] != '\0' || i + j >= jsonSize || json[i + j] == '\0') {
							//ERROR: Text end found, but value type was not text
							return -(i + j);
						}
						if (onItemFound != NULL) {
							if (!onItemFound(valueType, key, keyLength, Number(false), depth, index, owner_ptr)) {
								return -i; //ERROR: user error
							}
						}
					}
					i = i + j - 1;
				}
				step = 5; //End of value found
			}
			else {
				bool isFirstStep = (isObject && step == 0) || (!isObject && step == 3);

				//Characters outside of text
				bool isKeyValSep = step == 2 && c == ':';
				bool isItemSep = step == 5 && c == ',';

				//bool isObjectStart = (step == 0 || step == 3) && c == '{';
				bool isObjectStart = step == 3 && c == '{';
				bool isObjectEnd = (isFirstStep || step == 5) && c == '}';
				bool isArrayStart = step == 3 && c == '[';
				bool isArrayEnd = (isFirstStep || step == 5) && c == ']';

				if (isKeyValSep) {
					step++;
				}
				else if (isItemSep) {
					if (isObject) {
						step = 0;
					}
					else {
						step = 3;
					}
					index++;
				}
				else if (isObjectStart) {
					valueType = JSONItemType::JIT_ObjectBegin;
					if (onObjArrFound != NULL) {
						if (!onObjArrFound(valueType, key, keyLength, depth, index, owner_ptr)) {
							return -i; //ERROR: user error
						}
					}
					int pRes = parseObjArr(json + i + 1, jsonSize - i - 1, true, key, keyLength, depth + 1, owner_ptr);
					if (pRes <= 0) {
						//Parsing ERROR
						return pRes - i;
					}
					valueType = JSONItemType::JIT_ObjectEnd;
					if (onObjArrFound != NULL) {
						if (!onObjArrFound(valueType, key, keyLength, depth, index, owner_ptr)) {
							return -i; //ERROR: user error
						}
					}
					i += pRes;
					step = 5; //End of value found
				}
				else if (isObjectEnd && isObject) {
					return i + 1; //Object is finally parsed
				}
				else if (isArrayStart) {
					valueType = JSONItemType::JIT_ArrayBegin;
					if (onObjArrFound != NULL) {
						if (!onObjArrFound(valueType, key, keyLength, depth, index, owner_ptr)) {
							return -i; //ERROR: user error
						}
					}
					int pRes = parseObjArr(json + i + 1, jsonSize - i - 1, false, key, keyLength, depth + 1, owner_ptr);
					if (pRes <= 0) {
						//Parsing ERROR
						return pRes - i;
					}
					valueType = JSONItemType::JIT_ArrayEnd;
					if (onObjArrFound != NULL) {
						if (!onObjArrFound(valueType, key, keyLength, depth, index, owner_ptr)) {
							return -i; //ERROR: user error
						}
					}
					i += pRes;
					step = 5; //End of value found
				}
				else if (isArrayEnd && !isObject) {
					return i + 1; //Aray is finally parsed
				}
				else if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
					//ERROR: unexpected character found outside of text
					return -i;
				}
			}
		}
	}
	return -i; //Parsing failed
}

void SimpleJSONParser::unescapeAndCopy(char* destStr, int destSize, const char* sourceStr, int sourceSize) {
	if (destSize <= 0) return;
	destSize--;
	bool isEscaped = false;
	int destPos = 0;
	for (int i = 0; destPos < destSize && i < sourceSize && sourceStr[i] != '\0'; i++) {
		char c = sourceStr[i]; //Real character
		char ec = c; //Escaped character

		//Escaping characters
		if (isEscaped) {
			switch (c) {
			case '0': ec = '\0'; break;
			case 'b': ec = '\b'; break;
			case 'f': ec = '\f'; break;
			case 'n': ec = '\n'; break;
			case 'r': ec = '\r'; break;
			case 't': ec = '\t'; break;
			//case 'u': break; //TODO unicode character
			}
			isEscaped = false;
		}
		else if (c == '\\') {
			isEscaped = true;
			//continue;
		}

		if (!isEscaped) { //Skip backslash
			destStr[destPos] = ec;
			destPos++;
		}
	}
	destStr[destPos] = 0; //Null terminator
}

#ifdef ARDUINO
String SimpleJSONParser::unescape(const char* sourceStr, int sourceSize) {
	String ret;
#else
std::string SimpleJSONParser::unescape(const char* sourceStr, int sourceSize) {
	std::string ret;
#endif // ARDUINO
	ret.reserve(sourceSize);

	bool isEscaped = false;
	for (int i = 0; i < sourceSize && sourceStr[i] != '\0'; i++) {
		char c = sourceStr[i]; //Real character
		char ec = c; //Escaped character

		//Escaping characters
		if (isEscaped) {
			switch (c) {
			case '0': ec = '\0'; break;
			case 'b': ec = '\b'; break;
			case 'f': ec = '\f'; break;
			case 'n': ec = '\n'; break;
			case 'r': ec = '\r'; break;
			case 't': ec = '\t'; break;
			//case 'u': break; //TODO unicode character
			}
			isEscaped = false;
		}
		else if (c == '\\') {
			isEscaped = true;
			//continue;
		}

		if (!isEscaped) { //Skip backslash
			ret += ec;
		}
	}

#ifdef ARDUINO
	ret.reserve(ret.length());
#else
	ret.shrink_to_fit();
#endif // ARDUINO
	return ret;
}

const char* SimpleJSONParser::null = "null";
const char* SimpleJSONParser::True = "true";
const char* SimpleJSONParser::False = "false";
