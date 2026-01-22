#pragma once
#include "Connection.hpp"
#include <cstddef>
#include <exception>
#include <istream>
#include <iterator>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/url.hpp> 
#include <unordered_set>


using SessionID =int; 
using HttpCode = int; 
using Endpoint = std::string; 
using Callback = std::function<bool(int, std::string, std::string)> ;

namespace beast = boost::beast; 
namespace http = beast::http; 
namespace url = boost::urls; 
namespace json = boost::json; 


struct HttpSession : public std::enable_shared_from_this<HttpSession>{
    
    
    public: 
        explicit HttpSession(tcp::socket socket, std::unordered_map<std::string, Callback> &callbacks, std::unordered_map<SessionID, bool> &awaitingResponse, int sid)
        : socket(std::move(socket)), callbackMap(callbacks), awaitingResponse(awaitingResponse), sessionID(sid){}

        void start(){
            readRequest(); 
        }

        void writeRequest(int responseCode, std::string responseJson){

            http::status jamar = responseCode == 200 ? http::status::ok : http::status::bad_request; 

            auto res = std::make_shared<http::response<http::string_body>>(
                jamar, 11); 
            
            res->set(http::field::server, "Blueshift Server Device"); 
            res->set(http::field::content_type, "text/plain"); 
            res->keep_alive(req.keep_alive()); 
            res->body() = responseJson; 
            res->prepare_payload(); 
            res->set(http::field::access_control_allow_origin, "*"); // Allow all origins
            res->set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
            res->set(http::field::access_control_allow_headers, "Content-Type");


            auto self = shared_from_this(); 
            http::async_write(socket, *res,
                [self, res](boost::system::error_code ec, std::size_t){
                self->close(); 
            });
        }


        void close(){
            boost::system::error_code ignored_ec; 
            socket.shutdown(tcp::socket::shutdown_send, ignored_ec); 
            // marks that the session is ready for removal from the vector
            isConnected = false; 
        }


    private: 
        tcp::socket socket;
        beast::flat_buffer buffer;  
        std::unordered_map<std::string, std::function<bool(int, std::string, std::string)>> &callbackMap; 
        std::unordered_map<SessionID, bool> &awaitingResponse; 
        http::request<http::string_body> req; 
        int sessionID; 
        bool isConnected = true; 

        std::string getEndpoint(std::string target){
            int idex = target.find("?"); 
            if(idex == std::string::npos){
                return target; 
            }
            else{
                return target.substr(0, idex); 
            }
        }


        void readRequest(){ 
            auto self = shared_from_this(); 
            http::async_read(socket, buffer, req, 
                [self](boost::system::error_code ec, std::size_t bytes_transferred){
                    if(!ec){
                        self->handleRequest(); 
                    }
                    else{
                        if(ec != http::error::end_of_stream){
                            std::cout<<"Read error: "<<ec.message()<<std::endl; 
                            self->close(); 
                        }

                        std::cerr << "Read Error Encountered: "<<ec.message()<<std::endl; 
                    }
                });
        }



        void handleRequest(){
            std::string methodString = req.method_string(); 
            std::string targetString = req.target(); 
            
            Endpoint endpt = getEndpoint(targetString); 

            std::string body;
            json::object obj; 
            if(methodString == "POST"){
                body = req.body(); 
            }
            else if(methodString == "GET"){
                try{
                    url::url_view parsedUrl(targetString); 
                    for(auto const& param : parsedUrl.params()){
                        if(param.has_value){    
                            obj[param.key] = param.value; 
                        }
                    }
                    body = json::serialize(obj); 
                }
                catch(std::exception e){
                    this->writeRequest(403, "Unparsable query string"); 
                    return; 
                }
            }
            else if(methodString == "OPTIONS"){
                // Preflight CORS response
                this->writeRequest(200, ""); 
                return; // stop further processing
            } 

            if(this->callbackMap.contains(endpt)){
                this->awaitingResponse[this->sessionID] = true; 
                this->callbackMap.at(endpt)(sessionID, methodString, body); 
            }
            else{
                this->writeRequest(403, "Target " + endpt + " Unavailable"); 
            }
        }
    };



class HttpListener : public std::enable_shared_from_this<HttpListener>{


    private: 

        // crated the opera
        inline static bool created; 
        inline static std::shared_ptr<HttpListener> operatingServer; 
        std::atomic<int> sessionCount{0};
    
        boost::asio::io_context ctx; 
        std::unordered_map<SessionID, std::shared_ptr<HttpSession>> sessionMap; 
        std::unordered_map<Endpoint, Callback> endpointMap; 
        std::unordered_map<SessionID, bool> sessionWaitingOnResponse;
        std::mutex mut; 
     

        tcp::acceptor acceptor; 
        tcp::endpoint endpoint;
        std::atomic<bool> isRunning; 
        std::vector<std::thread> threads;   

        void fail(beast::error_code ec, const char* what) {
            std::cerr << what << " error: " << ec.message() << "\n";
        }

        void doAccept() {
            acceptor.async_accept(
                boost::asio::make_strand(ctx), 
                    [self = shared_from_this()](beast::error_code ec, tcp::socket socket){
                        if(!ec){
                            {
                                std::lock_guard<std::mutex> lock(self->mut); 
                                self->sessionMap[self->sessionCount] = std::make_shared<HttpSession>(std::move(socket), self->endpointMap, self->sessionWaitingOnResponse ,self->sessionCount); 
                                self->sessionMap[self->sessionCount]->start(); 
                                self->sessionCount++; 
                            }
                        }
                        else{
                            self->fail(ec, "accept"); 
                        }

                        if(self->isRunning){
                            self->doAccept(); 
                        }
                   }
            ); 
        }


    public:
    
        static std::shared_ptr<HttpListener> createServer(unsigned short portNum){
            if(!HttpListener::created){
                auto server = std::make_shared<HttpListener>(portNum); 
                HttpListener::operatingServer = server; 
                HttpListener::created = true; 
                server->start(); 
            }
            return HttpListener::operatingServer;  
        }


        HttpListener(unsigned short portNum)
        : 
          acceptor(boost::asio::make_strand(ctx)),
          endpoint(tcp::endpoint{boost::asio::ip::make_address("0.0.0.0"), portNum}),
          isRunning(false)
        {}
    

        void write(int sessionID, std::string responseJson, HttpCode code){
            if(this->sessionMap.contains(sessionID)){
                if(sessionWaitingOnResponse.at(sessionID)){
                    this->sessionMap.at(sessionID)->writeRequest(code, responseJson); 
                    sessionWaitingOnResponse.at(sessionID) = false;
                }
                else{
                    std::cerr<<"Warning: multiple requests detected for single response"<<std::endl; 
                }
            }   
            else{
                std::cerr<<"Write attempt to unknown session ID"<<std::endl; 
            }
            
        }  

        std::unordered_map<SessionID, std::shared_ptr<HttpSession>>& getMap(){
            return this->sessionMap;
        }

        void addHttpWatch(Endpoint &endpt, std::function<bool(int64_t, std::string, std::string)> callback){
            this->endpointMap[endpt] = callback; 
        }   

        void removeHttpWatch(Endpoint &endpt){
            std::lock_guard<std::mutex> lk(this->mut); 
            this->endpointMap.erase(endpt); 
        }


        void removeSession(SessionID id){
            std::lock_guard<std::mutex> lk(this->mut); 
            this->sessionMap.erase(id); 
        }



        void start(){
            if(isRunning.exchange(true)) return; 

            beast::error_code ec; 
            acceptor.open(endpoint.protocol(), ec);
            if (ec) fail(ec, "open");
            acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
            if (ec) fail(ec, "set_option");
            acceptor.bind(endpoint, ec);
            if (ec) fail(ec, "bind");
            acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
            if (ec) fail(ec, "listen");

            doAccept(); 

            // start the thread pool: 

            for(int i = 0 ; i < 4; i++){
                threads.emplace_back([self = shared_from_this()](){
                    self->ctx.run(); 
                }); 
            }

            std::cout<<"Server has begin listening"<<std::endl; 
        }

        void stop(){
            if(!isRunning.exchange(false)) return; 
            beast::error_code ec; 
            acceptor.close(ec); 
            this->ctx.stop(); 

            for(auto& t : threads){
                if(t.joinable()) t.join(); 
            }

            threads.clear(); 

            std::cout<<"Server closed"<<std::endl; 
        }

}; 
