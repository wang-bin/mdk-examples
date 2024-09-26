#include "mdk/Player.h"

// https://stackoverflow.com/a/50151753
#define BOOST_NO_EXCEPTIONS
#include <boost/throw_exception.hpp>
void boost::throw_exception(std::exception const&) {}
void boost::throw_exception(std::exception const&, boost::source_location const&) {}

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <iostream>

typedef websocketpp::client<websocketpp::config::asio_client> client;

using namespace MDK_NS;

int main(int argc, char* argv[]) {
    Player p;
    p.setMedia("stream:");
    p.set(State::Playing);

    client c;

    std::string uri = "ws://127.0.0.1/live/0.live.flv";

    if (argc == 2) {
        uri = argv[1];
    }

	try {
        // Set logging to be pretty verbose (everything except message payloads)
        c.set_access_channels(websocketpp::log::alevel::all);
        c.clear_access_channels(websocketpp::log::alevel::frame_payload);
        c.set_error_channels(websocketpp::log::elevel::all);

        // Initialize ASIO
        c.init_asio();

        // Register our message handler
        c.set_message_handler([&](websocketpp::connection_hdl, client::message_ptr msg) {
            p.appendBuffer((uint8_t*)msg->get_payload().data(), msg->get_payload().size());
        });

        websocketpp::lib::error_code ec;
        client::connection_ptr con = c.get_connection(uri, ec);
        if (ec) {
            std::cout << "could not create connection because: " << ec.message() << std::endl;
            return 0;
        }

        // Note that connect here only requests a connection. No network messages are
        // exchanged until the event loop starts running in the next line.
        c.connect(con);

        // Start the ASIO io_service run loop
        // this will cause a single connection to be made to the server. c.run()
        // will exit when this connection is closed.
        c.run();
    } catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
    }
}