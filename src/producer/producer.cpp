#include "common.h"
#include <atomic>
#include <memory>
#include <thread>

using asio::ip::tcp;

class session : public std::enable_shared_from_this<session> {
public:
    explicit session(tcp::socket socket)
        : socket_(std::move(socket)) {}

    void start() { do_read(); }

private:
    void do_read() {
        auto self = shared_from_this();
        asio::async_read_until(socket_, buffer_, '\n',
            [this, self](std::error_code ec, std::size_t bytes_transferred) {
                if (ec) {
                    if (ec != asio::error::operation_aborted && ec != asio::error::eof) {
                        std::cerr << "Session read error: " << ec.message() << "\n";
                    }
                    return; // session ends
                }
                std::string line; line.resize(bytes_transferred);
                buffer_.sgetn(line.data(), bytes_transferred);
                if (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
                    // Normalize line endings: strip trailing CR/LF
                    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
                }
                if (line == "REQ") {
                    auto uuid = generate_uuid_v4();
                    auto response = uuid + "\n";
                    do_write(std::move(response));
                } else {
                    auto response = std::string("ERR unknown\n");
                    do_write(std::move(response));
                }
            });
    }

    void do_write(std::string response) {
        auto self = shared_from_this();
        asio::async_write(socket_, asio::buffer(response),
            [this, self, response = std::move(response)](std::error_code ec, std::size_t) {
                if (ec) {
                    if (ec != asio::error::operation_aborted) {
                        std::cerr << "Session write error: " << ec.message() << "\n";
                    }
                    return;
                }
                // Ready for next request
                do_read();
            });
    }

    tcp::socket socket_;
    asio::streambuf buffer_;
};

class server {
public:
    server(asio::io_context& io, const tcp::endpoint& ep)
        : acceptor_(io), socket_(io) {
        std::error_code ec;
        acceptor_.open(ep.protocol(), ec);
        if (ec) throw std::runtime_error("open: " + ec.message());
        acceptor_.set_option(asio::socket_base::reuse_address(true));
        acceptor_.bind(ep, ec);
        if (ec) throw std::runtime_error("bind: " + ec.message());
        acceptor_.listen(asio::socket_base::max_listen_connections, ec);
        if (ec) throw std::runtime_error("listen: " + ec.message());
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(socket_, [this](std::error_code ec) {
            if (!ec) {
                std::make_shared<session>(std::move(socket_))->start();
            } else {
                std::cerr << "Accept error: " << ec.message() << "\n";
            }
            do_accept();
        });
    }

    tcp::acceptor acceptor_;
    tcp::socket socket_;
};

int main(int argc, char* argv[]) {
    try {
        if (argc < 2 || argc > 3) {
            std::cerr << "Usage: producer <port> [threads]\n";
            return 1;
        }
        unsigned short port = static_cast<unsigned short>(std::stoi(argv[1]));
        int threads = (argc == 3) ? std::max(1, std::stoi(argv[2])) : static_cast<int>(std::thread::hardware_concurrency());
        if (threads <= 0) threads = 1;

        asio::io_context io;
        server srv(io, tcp::endpoint(tcp::v4(), port));

#if !defined(_WIN32)
        // Graceful shutdown with SIGINT/SIGTERM on POSIX
        asio::signal_set signals(io, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto){ io.stop(); });
#endif

        std::vector<std::thread> pool;
        for (int i = 0; i < threads - 1; ++i) {
            pool.emplace_back([&]{ io.run(); });
        }
        std::cout << "Producer running on port " << port << " with " << threads << " threads.\n";
        io.run();
        for (auto& t : pool) t.join();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
}