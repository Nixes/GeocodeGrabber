#include "stdafx.h"

#include <string>
#include <fstream>
#include <istream>
#include <iostream>

#include <cpprest/http_client.h>
#include <cpprest/filestream.h>

using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams

class GeocodeGrabber {
private:
	std::string api_key;

	double longitude;
	double latitude;

	std::string formatted_address;

	void ParseLongLat(json::value const &value) {
		std::cout << "ParseLongLat ran\n";
		if (!value.is_null()) {
			auto results = value.at(U("results")).at(0); // we only want the first result

			try {
				auto geometry = results.at(U("geometry"));
				auto formatted_address_t = results.at(U("formatted_address")).as_string();

				// convert this address into something usefull, then print to console
				formatted_address = utility::conversions::to_utf8string(formatted_address_t);
			} catch (json::json_exception const & e) {
				std::cout << "Exception parsing google api result " << e.what() << std::endl;
			}
		}
	}

	void GetLongLatFromAddress(std::string address) {
		// Create http_client to send the request.
		http_client client(U("https://maps.googleapis.com/maps/api/geocode/json"));

		// Build request URI and start the request.
		uri_builder builder = uri_builder();
		builder.append_query(U("address"), address.c_str() );
		builder.append_query(U("key"), api_key.c_str() );

		// print the raw received response
		client
			.request(methods::GET, builder.to_string())
			// continue when the response is available
			.then([](http_response response) -> pplx::task <utility::string_t> {
				// if the status is OK extract the body of the response into a JSON value
				// works only when the content type is application\json
				if (response.status_code() == status_codes::OK) {
					//return response.extract_json();				
					return response.extract_string();
				}

			})
			// continue when the JSON value is available
			.then([](pplx::task<utility::string_t> previousTask) {
				// get the JSON value from the task and display content from it
				try {
					std::string tmp_string = utility::conversions::to_utf8string( previousTask.get() );
					std::cout << tmp_string;
					//json::value const & v = previousTask.get();
					// do something with extracted value
				} catch (http_exception const & e) {
					//std::cout << e.what() << std::endl;
				}
			})
			.wait();

		// do another request this time, but this time obtain values from json
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
					ParseLongLat(v);
				}
				catch (http_exception const & e) {
					std::cout << e.what() << std::endl;
				}
			})
			.wait();
	}

public:
	GeocodeGrabber(std::string tmp_api_key) {
		api_key = tmp_api_key;
	}

	void printPrivate() {
		std::cout << "Formatted address was: " << formatted_address << std::endl;
		std::cout << "logitude: " << longitude << " latitude: " << latitude << std::endl;
	}

	void testApi() {
		std::string address = "John St, Hawthorn VIC";
		GetLongLatFromAddress(address);
	}

};

int main() {
	GeocodeGrabber geocode_test = GeocodeGrabber("AIzaSyD - NPqot8WGQyK0GtcrkMasHPIzKHB - HTo");

	geocode_test.testApi();

	system("pause");
	return 0;
}