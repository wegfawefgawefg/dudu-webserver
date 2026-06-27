#pragma once

#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <ctime>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace httplib {

inline std::atomic_bool running = true;

inline void handle_signal(int) {
    running = false;
}

class Fd {
public:
    Fd() = default;
    explicit Fd(int fd) : fd_(fd) {}

    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;

    Fd(Fd&& other) noexcept : fd_(std::exchange(other.fd_, -1)) {}

    Fd& operator=(Fd&& other) noexcept {
        if (this != &other) {
            close();
            fd_ = std::exchange(other.fd_, -1);
        }
        return *this;
    }

    ~Fd() {
        close();
    }

    int get() const {
        return fd_;
    }

    explicit operator bool() const {
        return fd_ >= 0;
    }

    void close() {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

private:
    int fd_ = -1;
};

struct Request {
    std::string method;
    std::string target;
    std::string path;
    std::map<std::string, std::string> query;

    std::string get_param_value(const char* key) const {
        auto found = query.find(key);
        if (found == query.end()) {
            return {};
        }
        return found->second;
    }
};

struct Response {
    int status = 200;
    std::string body;
    std::string content_type = "text/plain; charset=utf-8";

    void set_content(const std::string& next_body, const char* next_content_type) {
        body = next_body;
        content_type = next_content_type;
    }

    void set_content(const char* next_body, const char* next_content_type) {
        body = next_body;
        content_type = next_content_type;
    }
};

inline int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

inline std::string url_decode(std::string_view input) {
    std::string out;
    out.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '+') {
            out.push_back(' ');
        } else if (input[i] == '%' && i + 2 < input.size()) {
            const int high = hex_value(input[i + 1]);
            const int low = hex_value(input[i + 2]);
            if (high >= 0 && low >= 0) {
                out.push_back(static_cast<char>(high * 16 + low));
                i += 2;
            } else {
                out.push_back(input[i]);
            }
        } else {
            out.push_back(input[i]);
        }
    }
    return out;
}

inline std::map<std::string, std::string> parse_query(std::string_view query) {
    std::map<std::string, std::string> params;
    std::size_t start = 0;
    while (start <= query.size()) {
        const auto end = query.find('&', start);
        const auto pair = query.substr(start, end == std::string_view::npos ? query.size() - start : end - start);
        if (!pair.empty()) {
            const auto equals = pair.find('=');
            const auto key = pair.substr(0, equals);
            const auto value = equals == std::string_view::npos ? std::string_view{} : pair.substr(equals + 1);
            params[url_decode(key)] = url_decode(value);
        }
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1;
    }
    return params;
}

inline Request parse_request(std::string_view raw) {
    Request request;
    const auto first_line_end = raw.find("\r\n");
    if (first_line_end == std::string_view::npos) {
        return request;
    }

    std::istringstream first_line(std::string(raw.substr(0, first_line_end)));
    std::string version;
    first_line >> request.method >> request.target >> version;

    const auto question = request.target.find('?');
    if (question == std::string::npos) {
        request.path = request.target;
    } else {
        request.path = request.target.substr(0, question);
        request.query = parse_query(std::string_view(request.target).substr(question + 1));
    }
    return request;
}

inline std::string read_request(int fd) {
    std::string raw;
    raw.reserve(4096);

    char buffer[1024]{};
    while (raw.find("\r\n\r\n") == std::string::npos && raw.size() < 16384) {
        const ssize_t count = ::recv(fd, buffer, sizeof(buffer), 0);
        if (count <= 0) {
            break;
        }
        raw.append(buffer, static_cast<std::size_t>(count));
    }
    return raw;
}

inline std::string reason_phrase(int status) {
    switch (status) {
    case 200: return "OK";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    default: return "OK";
    }
}

inline std::string serialize_response(const Response& response) {
    std::ostringstream out;
    out << "HTTP/1.1 " << response.status << ' ' << reason_phrase(response.status) << "\r\n";
    out << "Server: dudu-webserver\r\n";
    out << "Connection: close\r\n";
    out << "Content-Type: " << response.content_type << "\r\n";
    out << "Content-Length: " << response.body.size() << "\r\n";
    out << "\r\n";
    out << response.body;
    return out.str();
}

inline void send_all(int fd, std::string_view data) {
    while (!data.empty()) {
        const ssize_t sent = ::send(fd, data.data(), data.size(), MSG_NOSIGNAL);
        if (sent <= 0) {
            return;
        }
        data.remove_prefix(static_cast<std::size_t>(sent));
    }
}

inline std::string time_json() {
    std::time_t now = std::time(nullptr);
    std::tm tm{};
    gmtime_r(&now, &tm);

    char buffer[32]{};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);

    std::ostringstream out;
    out << "{\"epoch\":\"" << now << "\",\"utc\":\"" << buffer << "\"}";
    return out.str();
}

inline std::string hello_json(const std::string& name) {
    return "{\"message\":\"hello, " + name + "\"}";
}

inline int env_port_or(int fallback) {
    const char* text = std::getenv("PORT");
    if (text == nullptr || text[0] == '\0') {
        return fallback;
    }
    const int parsed = std::atoi(text);
    if (parsed <= 0) {
        return fallback;
    }
    return parsed;
}

class Server {
public:
    template <typename Handler>
    void Get(const char* path, Handler handler) {
        routes_.push_back(Route{path, handler});
    }

    void listen(const char* host, int port) {
        std::signal(SIGINT, handle_signal);
        std::signal(SIGTERM, handle_signal);

        Fd server(::socket(AF_INET, SOCK_STREAM, 0));
        if (!server) {
            std::cerr << "socket failed: " << std::strerror(errno) << "\n";
            return;
        }

        int yes = 1;
        ::setsockopt(server.get(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(static_cast<uint16_t>(port));
        if (::inet_pton(AF_INET, host, &address.sin_addr) != 1) {
            std::cerr << "invalid host: " << host << "\n";
            return;
        }

        if (::bind(server.get(), reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
            std::cerr << "bind failed: " << std::strerror(errno) << "\n";
            return;
        }

        if (::listen(server.get(), 64) < 0) {
            std::cerr << "listen failed: " << std::strerror(errno) << "\n";
            return;
        }

        std::cout << "dudu-webserver listening on http://" << host << ":" << port << "\n";
        std::cout << "press Ctrl-C to stop\n";

        while (running) {
            pollfd server_poll{
                .fd = server.get(),
                .events = POLLIN,
                .revents = 0,
            };
            const int poll_result = ::poll(&server_poll, 1, 200);
            if (poll_result <= 0 || (server_poll.revents & POLLIN) == 0) {
                continue;
            }

            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            Fd client(::accept(server.get(), reinterpret_cast<sockaddr*>(&client_addr), &client_len));
            if (!client) {
                continue;
            }
            serve_client(client.get());
        }
    }

private:
    struct Route {
        std::string path;
        std::function<void(Request, Response&)> handler;
    };

    std::vector<Route> routes_;

    void serve_client(int fd) {
        const std::string raw = read_request(fd);
        Request request = parse_request(raw);
        Response response;

        if (request.method != "GET") {
            response.status = 405;
            response.set_content("{\"error\":\"method_not_allowed\"}", "application/json");
            send_all(fd, serialize_response(response));
            return;
        }

        for (const auto& route : routes_) {
            if (route.path == request.path) {
                route.handler(request, response);
                send_all(fd, serialize_response(response));
                return;
            }
        }

        response.status = 404;
        response.set_content("{\"error\":\"not_found\"}", "application/json");
        send_all(fd, serialize_response(response));
    }
};

} // namespace httplib
