#include "stdafx.h"

#include "GeocodeGrabber.hpp"

int main() {
	GeocodeGrabber geocode_test = GeocodeGrabber("AIzaSyD - NPqot8WGQyK0GtcrkMasHPIzKHB - HTo", "AIzaSyBF70jGFpFNUGJFMUOqVLQfTikvKRrdc0U");

	// to specify a location based on an address use
	std::string address = "John St, Hawthorn VIC";
	geocode_test.GetLongLatFromAddress(address);

	// to guestimate the users location based on their ip use
	//geocode_test.GetLongLatFromIp();

	// get the resulting Sunrise and Sunset times as doubles
	std::cout << "Sunrise: " << geocode_test.GetSunrise() << std::endl;
	std::cout << std::endl;
	std::cout << "Sunset: " << geocode_test.GetSunset() << std::endl;

	system("pause");
	return 0;
}