#include "http_server.h"


namespace http_server {

	void ReportError(beast::error_code ec, std::string_view what) {
		std::cerr << what << ": "sv << ec.message() << std::endl;
	}

	void SessionBase::Run() {
		net::dispatch(stream_.get_executor(), beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
	}

}  // namespace http_server
