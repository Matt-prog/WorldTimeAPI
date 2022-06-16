# WorldTimeAPI
C++ library with client side of [WorldTimeAPI](http://worldtimeapi.org).
> WorldTimeAPI is a simple web service which returns the current local time for a given timezone as either plain-text or JSON.

## Supported OS/MCUs:
- **Windows**
- **Linux**
- **Mac OS**
- **ESP8266** with arduino core
- **ESP32** with arduino core

## How to use
There are three main functions:
- `getByIP()` which retrieves time and time zone informations from specified public IP address (only IPv4). If IP address is not specified, operation will be done for your public IP address.
- `getByTimeZone()` which retrieves time and time zone informations by specified olson time zone name.
- `getListOfTimeZones()` which gets list of all supported olson time zone names.
Those functions are blocking, so code is stopped until response from API is received. On ESP32 and ESP8266 there is 1s timeout for receiving response.

## Dependecies
This library uses multiplatform DateTime library for C++. It has to be included to your project/solution.

## Examples
Example for Windows, Linux and Mac OS:
```
#include <iostream>
#include <thread>
  
#include "DateTime.h"
#include "WorldTimeAPI.h"

int main()
{
	WorldTimeAPI api;
	auto res = api.getByIP(); //Gets current time and time zone based on your public IP address.

	while (true) {
		std::cout << res.datetime.toString() << std::endl;    //Print current time obtained from WorldTimeAPI
		//Time accuracy obtained from WorldTimeAPI is usually ±1 second.
    
		std::this_thread::sleep_for(std::chrono::seconds(1)); //Wait for one second
	}

	return 0;
}
```
ESP32 and ESP8266 examples are located in `examples` folder.

## Author
👤 [Matej Fitoš](https://github.com/Matt-prog)

## Thanks
Thanks to author of WorldTimeAPI for his brilliant idea.

## License
Copyright © 2022 [Matej Fitoš](https://github.com/Matt-prog).

This project is [MIT](https://github.com/Matt-prog/WorldTimeAPI/blob/main/LICENSE) licensed.
