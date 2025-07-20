#include "common.h"
#include <chrono>
#include <thread>

using asio::ip::tcp;

int main(int argc, char* argv[]) {
    try {
        if (argc < 3 || argc > 5) {
            std::cerr << "Usage: consumer <host> <port> [count=1] [delay_ms=0]\n";
            return 1;
        }
        std::string host = argv[1];
        unsigned short port = static_cast<unsigned short>(std::stoi(argv[2]));
        int count = (argc >= 4) ? std::max(1, std::stoi(argv[3])) : 1;
        int delay_ms = (argc == 5) ? std::max(0, std::stoi(argv[4])) : 0;

        asio::io_context io;
        tcp::resolver resolver(io);
        auto endpoints = resolver.resolve(host, std::to_string(port));

        tcp::socket socket(io);
        asio::connect(socket, endpoints);
        std::cout << "Connected to " << host << ':' << port << "\n";

        asio::streambuf buf;
        for (int i = 0; i < count; ++i) {
            std::string req = "REQ\n";
            asio::write(socket, asio::buffer(req));
            asio::read_until(socket, buf, '\n');
            std::istream is(&buf);
            std::string uuid; std::getline(is, uuid); // strip '\n'
            if (!uuid.empty() && uuid.back() == '\r') uuid.pop_back();
            if (uuid.rfind("ERR", 0) == 0) {
                std::cerr << "Server error: " << uuid << '\n';
            } else {
                std::cout << uuid << '\n';
            }
            if (delay_ms > 0) std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }

        // Optionally keep the connection open for interactive mode.
        // For this sample, we just close after the requested count.
        socket.close();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
}