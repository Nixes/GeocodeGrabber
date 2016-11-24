#include "stdafx.h"

#include <string>
#include <fstream>
#include <istream>
#include <iostream>
#include <ctime>

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

public:
	GeocodeGrabber(std::string tmp_geocoding_api_key, std::string tmp_geolocation_api_key) {
		geocoding_api_key = tmp_geocoding_api_key;
		geolocation_api_key = tmp_geolocation_api_key;
		longitude = 0.0;
		latitude = 0.0;
		formatted_address = "";
	}

	// return the predicted sunset time as decimal hours
	float GetSunset() {
	}

	// return the predicted sunrise time as decimal hours
	float GetSunrise() {

	}

	void PrintPrivate() {
		std::cout << "Formatted address was: " << formatted_address << std::endl;
		std::cout << "logitude: " << longitude << " latitude: " << latitude << std::endl;
	}

	void TestApi() {
		std::string address = "John St, Hawthorn VIC";
		GetLongLatFromAddress(address);
	}

	void TestIp() {
		GetLongLatFromIp();
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