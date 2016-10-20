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
	const std::string api_key = "AIzaSyD-NPqot8WGQyK0GtcrkMasHPIzKHB-HTo";

	double logitude;
	double latitude;

	std::string GetLongLatFromAddress(std::string address) {
		std::string return_string;

		// Create http_client to send the request.
		http_client client(U("https://maps.googleapis.com/maps/api/geocode/json"));

		// Build request URI and start the request.
		uri_builder builder = uri_builder();
		builder.append_query(U("address"), address.c_str() );
		builder.append_query(U("key"), api_key.c_str() );

		client
			.request(methods::GET, builder.to_string())
			// continue when the response is available
			.then([&return_string](http_response response) -> pplx::task <utility::string_t> {
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
					//std::string tmp_string = utility::conversions::to_utf8string( v.as_string() );
					//return_string = tmp_string;
				} catch (http_exception const & e) {
					//std::cout << e.what() << std::endl;
				}
			})
			.wait();
			return return_string;
	}

public:
	GeocodeGrabber() {}

	void testApi() {
		std::string address = "3793 Australia";
		std::cout << GetLongLatFromAddress(address);
	}

};

int main() {
	GeocodeGrabber geocode_test = GeocodeGrabber();

	geocode_test.testApi();

	system("pause");
	return 0;
}