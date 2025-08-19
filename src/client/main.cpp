#include "libCE/Client.hpp"
#include <functional>
#include <string>
#include <thread>
#include <unistd.h>
#ifdef SDL_ENABLED
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#endif


int main(int argc, char* argv[]) {
    std::string name;
    if (argc == 2) {
        name = argv[1];
    }
    else {        
        char hostname[HOST_NAME_MAX];
        gethostname(hostname, HOST_NAME_MAX+1);
        name = hostname;
    }
    auto client = Client(name);
    #ifdef SDL_ENABLED
        bool sdlRunning = true;
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            sdlRunning = false;
            std::cerr << "Failed to start SDL: " << SDL_GetError() << ". Some devices may not work properly." << std::endl;
        }

        SDL_Window* window = nullptr;
        SDL_Renderer* renderer = nullptr;
        auto windowName = name + " Input Window";
        if (!SDL_CreateWindowAndRenderer(windowName.c_str(), 800, 600, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY, &window, &renderer)) {
            sdlRunning = false;
            std::cerr << "Input window creation failed: " << SDL_GetError() << ". Some devices may not work properly." << std::endl;
        }
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);

        std::jthread clientEngine(std::bind(&Client::start, std::ref(client)));

        while (sdlRunning) {
            SDL_RenderClear(renderer);
            SDL_RenderPresent(renderer);
            SDL_Event event;
            SDL_WaitEvent(&event);
            switch (event.type) {
                case SDL_EVENT_QUIT: // for now closing the input window will stop the client
                    client.disconnect();
                    goto exit;
                break;
                case SDL_EVENT_USER: // if no SDL devices are created, stop the SDL subsystem
                    goto exit;
                break;
            }
        }
        exit: ;

        SDL_Quit();
        clientEngine.join();
    #else
        client.start();
    #endif
}


/*
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <string>
#include <sstream>

using boost::asio::ip::tcp;
namespace json = boost::json;

int main() {
    try {
        boost::asio::io_context io;

        // 1. Resolve the host and port
        tcp::resolver resolver(io);
        auto endpoints = resolver.resolve("jsonplaceholder.typicode.com", "80");

        // 2. Create and connect the socket
        tcp::socket socket(io);
        boost::asio::connect(socket, endpoints);

        // 3. Form the HTTP request
        std::string request =
            "GET /todos/1 HTTP/1.1\r\n"
            "Host: jsonplaceholder.typicode.com\r\n"
            "Connection: close\r\n\r\n";

        // 4. Send the request
        boost::asio::write(socket, boost::asio::buffer(request));

        // 5. Read the entire response into a string
        boost::asio::streambuf response;
        boost::system::error_code error;
        std::ostringstream ss;
        while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error)) {
            ss << &response;
        }
        if (error != boost::asio::error::eof) // eof means clean shutdown
            throw boost::system::system_error(error);

        std::string http_response = ss.str();
        std::cout<<http_response<<std::endl; 

        // 6. Extract the JSON body from the HTTP 
        
        
        std::string delimiter = "\r\n\r\n";
        size_t pos = http_response.find(delimiter);
        if (pos == std::string::npos) {
            std::cerr << "No body found in HTTP response\n";
            return 1;
        }
        std::string json_str = http_response.substr(pos + delimiter.length());

        std::cout << "Extracted JSON:\n" << json_str << "\n";
        

    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}
*/ 

/*
// async_asio_http.cpp
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/url.hpp> 
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <atomic>

namespace asio  = boost::asio;
namespace beast = boost::beast;
namespace http  = beast::http;
namespace url = boost::urls; 
using tcp       = asio::ip::tcp;

class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
    explicit HttpSession(tcp::socket socket)
        : socket_(std::move(socket))
    {}

    void start() {
        doRead();
    }


    

private:

    std::string getEndpoint(std::string target){
        int idex = target.find("?"); 
        if(idex == std::string::npos){
            return target; 
        }
        else{
            return target.substr(0, idex); 
        }
    }

    tcp::socket socket_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;

    void doRead() {
        auto self = shared_from_this();
        http::async_read(socket_, buffer_, req_,
            [self](beast::error_code ec, std::size_t bytes_transferred) {
                if (!ec) {
                    self->handleRequest();
                } else {
                    if(ec != http::error::end_of_stream)
                        std::cerr << "Read error: " << ec.message() << std::endl;
                        self->doClose();
                    }
            });
    }

    void handleRequest() {
     
        std::cout << "Method: " << req_.method_string() << "\n";
        std::cout << "Target: " << getEndpoint(req_.target())<< "\n";
        std::cout << "Body: " << req_.body() << "\n";

        url::url_view omar(req_.target()); 
        for(auto const& param : omar.params()){
            std::cout<<param.key<< " : "<<param.value<<std::endl;
        }
        
        // Make response a shared pointer to extend lifetime until async_write completes
        
        auto res = std::make_shared<http::response<http::string_body>>(
        http::status::ok, req_.version());
        res->set(http::field::server, "Beast");
        res->set(http::field::content_type, "text/plain");
        res->keep_alive(req_.keep_alive());
        res->body() = "Hello from Beast!";
        res->prepare_payload();
        
        auto self = shared_from_this();
        http::async_write(socket_, *res,
            [self, res](beast::error_code ec, std::size_t) {
                if (ec)
                    std::cerr << "Write error: " << ec.message() << "\n";
                self->doClose();
        });       
    }

    void doClose() {
        beast::error_code ec;
        socket_.shutdown(tcp::socket::shutdown_send, ec);
        // Ignore errors on shutdown
    }
};

class HttpServer : public std::enable_shared_from_this<HttpServer> {
public:
    HttpServer(asio::io_context& ioc, tcp::endpoint endpoint, std::size_t threadCount)
        : ioc_(ioc),
          acceptor_(asio::make_strand(ioc)),
          endpoint_(endpoint),
          threadCount_(threadCount),
          isRunning_(false)
    {}

    void start() {
        if (isRunning_.exchange(true)) return;

        beast::error_code ec;
        acceptor_.open(endpoint_.protocol(), ec);
        if (ec) fail(ec, "open");
        acceptor_.set_option(asio::socket_base::reuse_address(true), ec);
        if (ec) fail(ec, "set_option");
        acceptor_.bind(endpoint_, ec);
        if (ec) fail(ec, "bind");
        acceptor_.listen(asio::socket_base::max_listen_connections, ec);
        if (ec) fail(ec, "listen");

        doAccept();

        // Run thread pool
        for (std::size_t i = 0; i < threadCount_; ++i) {
            threads_.emplace_back([this] {
                ioc_.run();
            });
        }

        std::cout << "Server listening on " << endpoint_ << " with " << threadCount_ << " threads\n";
    }

    void stop() {
        if (!isRunning_.exchange(false)) return;
        beast::error_code ec;
        acceptor_.close(ec);
        ioc_.stop();

        for (auto& t : threads_) {
            if (t.joinable()) t.join();
        }
        threads_.clear();

        std::cout << "Server stopped.\n";
    }

private:
    asio::io_context& ioc_;
    tcp::acceptor acceptor_;
    tcp::endpoint endpoint_;
    std::size_t threadCount_;
    std::atomic<bool> isRunning_;
    std::vector<std::thread> threads_;

    void doAccept() {
        acceptor_.async_accept(
            asio::make_strand(ioc_),
            [self = shared_from_this()](beast::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<HttpSession>(std::move(socket))->start();
                } else {
                    self->fail(ec, "accept");
                }
                if (self->isRunning_) {
                    self->doAccept();
                }
            });
    }

    void fail(beast::error_code ec, const char* what) {
        std::cerr << what << " error: " << ec.message() << "\n";
    }
};

int main() {
    try {
        asio::io_context ioc{1};
        auto address = asio::ip::make_address("0.0.0.0");
        unsigned short port = 8080;
        std::size_t threads = std::thread::hardware_concurrency();

        auto server = std::make_shared<HttpServer>(ioc, tcp::endpoint{address, port}, threads);
        server->start();

        std::cout << "Press Enter to stop the server...\n";
        std::string line;
        std::getline(std::cin, line);

        server->stop();
    }
    catch (std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
    }
}
*/ 





