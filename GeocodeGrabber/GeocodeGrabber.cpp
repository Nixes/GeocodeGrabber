#include "stdafx.h"

#include <string>
#include <fstream>
#include <istream>
#include <iostream>
#include <ctime>
#define _USE_MATH_DEFINES
#include <math.h>

#include <cpprest/http_client.h>
#include <cpprest/filestream.h>

using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams

class GeocodeGrabber {
private:
	std::string geocoding_api_key; // key used for address -> long lat conversions
	std::string geolocation_api_key; // key used for request ip -> long lat conversions

	double longitude;
	double latitude;

	std::string formatted_address;

	void ParseLocation(json::value const &response) {
		if (response.has_field(U("location")) ) {
			try {
				auto location = response.at(U("location"));
				longitude = location.at(U("lng")).as_double();
				latitude = location.at(U("lat")).as_double();
			}
			catch (json::json_exception const & e) {
				std::cout << "Failed to parse location " << e.what() << std::endl;
			}
		}
	}

	void ParseGeocode(json::value const &value) {
		std::cout << "ParseLongLat ran\n";
		if (!value.is_null()) {
			auto results = value.at(U("results")).at(0); // we only want the first result

			try {
				auto formatted_address_t = results.at(U("formatted_address")).as_string();
				// convert this address into something usefull
				formatted_address = utility::conversions::to_utf8string(formatted_address_t);
			} catch (json::json_exception const & e) {
				std::cout << "Exception parsing google api result " << e.what() << std::endl;
			}
			ParseLocation(results.at(U("geometry")));

		} else {
			std::cout << "json was null" << std::endl;
		}
	}

	void GetLongLatFromAddress(std::string address) {
		// Create http_client to send the request.
		http_client client(U("https://maps.googleapis.com/maps/api/geocode/json"));

		// Build request URI and start the request.
		uri_builder builder = uri_builder();
		builder.append_query(U("address"), address.c_str() );
		builder.append_query(U("key"), geocoding_api_key.c_str() );

		client
			.request(methods::GET, builder.to_string())
			// continue when the response is available
			.then([](http_response response) -> pplx::task <json::value> {
				// if the status is OK extract the body of the response into a JSON value
				// works only when the content type is application\json
				if (response.status_code() == status_codes::OK) {
					return response.extract_json();				
				}
				return pplx::task_from_result(json::value());
			})
				// continue when the JSON value is available
			.then([this](pplx::task<json::value> previousTask) {
				// get the JSON value from the task and display content from it
				try {
					json::value const & v = previousTask.get();
					ParseGeocode(v);
				}
				catch (http_exception const & e) {
					std::cout << e.what() << std::endl;
				}
			})
			.wait();
	}

	void GetLongLatFromIp() {
		// Create http_client to send the request.
		http_client client(U("https://www.googleapis.com/geolocation/v1/geolocate"));

		// Build request URI and start the request.
		uri_builder builder = uri_builder();
		builder.append_query(U("key"), geolocation_api_key.c_str());

		client
			.request(methods::POST, builder.to_string())
			// continue when the response is available
			.then([](http_response response) -> pplx::task <json::value> {
			// if the status is OK extract the body of the response into a JSON value
			// works only when the content type is application\json
			if (response.status_code() == status_codes::OK) {
				return response.extract_json();
			}
			return pplx::task_from_result(json::value());
		})
			// continue when the JSON value is available
			.then([this](pplx::task<json::value> previousTask) {
			// get the JSON value from the task and display content from it
			try {
				json::value const & v = previousTask.get();
				ParseLocation(v);
			}
			catch (http_exception const & e) {
				std::cout << e.what() << std::endl;
			}
		})
			.wait();
	}

	// some simple variants of the built in trigonomic functions that use use degrees as parameters AND outputs
	double DegToRad(double degrees) {
		return degrees * (180 / M_PI);
	}
	double RadToDeg(double radians) {
		return radians * (180 / M_PI);
	}
	double d_cos(double degrees) {
		return RadToDeg(cos(DegToRad(degrees)));
	}
	double d_acos(double degrees) {
		return RadToDeg(acos(DegToRad(degrees)));
	}
	double d_sin(double degrees) {
		return RadToDeg(sin(DegToRad(degrees)));
	}
	double d_asin(double degrees) {
		return RadToDeg(asin(DegToRad(degrees)));
	}
	double d_tan(double degrees) {
		return RadToDeg(tan(DegToRad(degrees)));
	}
	double d_atan(double degrees) {
		return RadToDeg(atan(DegToRad(degrees)));
	}



	// What follows is a c++ implementation of the algorithm specified here: http://williams.best.vwh.net/sunrise_sunset_algorithm.htm

	// TODO: Improve variable names
	double GetDayOfYear() {
		time_t c_now = time(0);
		struct tm now;
		localtime_s(&now, &c_now);

		int a = floor(275 * now.tm_mon / 9);
		int b = floor( (now.tm_mon + 9) /12 );
		int c = (1 + floor((now.tm_year - 4 * floor(now.tm_year / 4) + 2) / 3));

		return a - (b * c) + now.tm_mday - 30;
	}

	float GetSunriseLongHour(int day_of_year) {
		float long_hour = longitude / 15;

		float rising_time = day_of_year + ((6 - long_hour) / 24);
		return rising_time;
	}

	float GetSunsetLongHour(int day_of_year) {
		float long_hour = longitude / 15;

		float setting_time = day_of_year + ((18 - long_hour) / 24);
		return setting_time;
	}

	float GetSunMeanAnomaly(float time) {
		return (0.9856 * time) - 3.289;
	}

	// this is where most of the magic happens: returns sunrise time if true, sunset if false.
	double GetLocalHours(bool calc_sunrise) {
		const double day_of_year = GetDayOfYear();

		double t = 0;
		if (calc_sunrise) {
			std::cout << "Calculating sunrise" << std::endl;
			t = GetSunriseLongHour(day_of_year);
		} else {
			std::cout << "Calculating sunset" << std::endl;
			t = GetSunsetLongHour(day_of_year);
		}
		std::cout << "t: " << t << std::endl;

		const double zenith = 90.83333; // official zenith

		double sun_mean_anomaly = GetSunMeanAnomaly(t);
		std::cout << "sun_mean_anomaly: " << sun_mean_anomaly << std::endl;

		double sun_true_longitude = sun_mean_anomaly + (1.916 * d_sin(sun_mean_anomaly)) + (0.020 * d_sin(2 * sun_mean_anomaly)) + 282.634;
		std::cout << "sun_true_longitude: " << sun_true_longitude << std::endl;

		double sun_right_ascension = d_atan(0.91764 * d_tan(sun_true_longitude));
		std::cout << "sun_right_ascension: " << sun_right_ascension << std::endl;

		double true_long_quadrant = (floor(sun_true_longitude / 90)) * 90;
		double right_ascension_quadrant = (floor(sun_right_ascension / 90)) * 90;
		sun_right_ascension = sun_right_ascension + (true_long_quadrant - right_ascension_quadrant);
		std::cout << "sun_right_ascension AFTER QUAD: " << sun_right_ascension << std::endl;

		// convert right ascension to hours
		double right_ascension_hours = sun_right_ascension / 15;
		std::cout << "right_ascension_hours: " << right_ascension_hours << std::endl;

		// calc sun declination
		double sine_declination = 0.39782 * sin(sun_true_longitude);
		double cosine_declination = d_cos(d_asin(sine_declination));
		std::cout << "cosine_declination: " << cosine_declination << std::endl;

		// calc sun local hour angle
		double cos_hour_angle = (d_cos(zenith) - (sine_declination * d_sin(latitude))) / (cosine_declination * d_cos(latitude));
		std::cout << "cos_hour_angle: " << cos_hour_angle << std::endl;

		// calculate hours
		double tmp_hours = 0;
		if (calc_sunrise) {
			tmp_hours = 360 - d_acos(cos_hour_angle);
		} else {
			tmp_hours = d_acos(cos_hour_angle);
		}
		std::cout << "tmp_hours: " << tmp_hours << std::endl;

		double hours = tmp_hours / 15;
		std::cout << "hours: " << hours << std::endl;

		// calc local mean time of sunset/sunrise
		double mean_sun_transition = hours + right_ascension_hours - (0.06571 * t) - 6.622;
		std::cout << "mean_sun_transition: " << mean_sun_transition << std::endl;

		return mean_sun_transition;
	}
public:
	GeocodeGrabber(std::string tmp_geocoding_api_key, std::string tmp_geolocation_api_key) {
		geocoding_api_key = tmp_geocoding_api_key;
		geolocation_api_key = tmp_geolocation_api_key;
		longitude = 0.0;
		latitude = 0.0;
		formatted_address = "";
	}

	// return the predicted sunset time as decimal hours
	double GetSunset() {
		return GetLocalHours(false);
	}

	// return the predicted sunrise time as decimal hours
	double GetSunrise() {
		return GetLocalHours(true);
	}

	void PrintPrivate() {
		std::cout << "Formatted address was: " << formatted_address << std::endl;
		std::cout << "logitude: " << longitude << " latitude: " << latitude << std::endl;

		std::cout << "Sunrise: " << GetSunrise() << std::endl;
		std::cout << "Sunset: " << GetSunset() << std::endl;
	}

	void TestApi() {
		std::string address = "John St, Hawthorn VIC";
		GetLongLatFromAddress(address);
	}

	void TestIp() {
		GetLongLatFromIp();
		std::cout << "Day of year: " << GetDayOfYear() << std::endl;
	}

};

int main() {
	GeocodeGrabber geocode_test = GeocodeGrabber("AIzaSyD - NPqot8WGQyK0GtcrkMasHPIzKHB - HTo", "AIzaSyBF70jGFpFNUGJFMUOqVLQfTikvKRrdc0U");

	//geocode_test.TestApi();

	geocode_test.TestIp();

	geocode_test.PrintPrivate();

	system("pause");
	return 0;
}