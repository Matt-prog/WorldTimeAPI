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

## Dependecies
This library uses multiplatform DateTime library for C++. It has to be included to your project/solution.

## Author
👤 [Matej Fitoš](https://github.com/Matt-prog)

## Thanks
Thanks to author of WorldTimeAPI for his brilliant idea.

## License
Copyright © 2022 [Matej Fitoš](https://github.com/Matt-prog).

This project is [MIT](https://github.com/Matt-prog/WorldTimeAPI/blob/main/LICENSE) licensed.
