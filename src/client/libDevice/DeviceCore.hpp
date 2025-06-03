#pragma once

#include "include/Common.hpp"
#include "libDM/DynamicMessage.hpp"
#include "libnetwork/Protocol.hpp"
#include <condition_variable>
#include <cstdint>
#include <fstream>
#include <functional>
#include <boost/asio.hpp>
#include <chrono> 
#include "libnetwork/Connection.hpp"
#include <mutex>
#include <sys/inotify.h>
#include <thread>
#include <unistd.h> 
#include <vector>

#define VOLATILITY_LIST_SIZE 10


/* 
    This file contains all the core device functions
    and utlity classes that can asist with the easy
    creation of devices. 


*/

using boost::asio::ip::tcp; 
using boost::asio::ip::udp; 


using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>; 

class DeviceInterruptor; 

// Configuration information used in sending types
enum class SrcType{
    UNIX_FILE, 
    GPIO, 
}; 


struct Interrupt_Desc{
        SrcType src_type; 
        std::function<bool()> handler; 
        std::string file_src;
        int port_num; 
}; 

inline bool sendState(){
    return true; 
}

class AbstractDevice{
    private: 
        std::vector<Interrupt_Desc> Idesc_list;
        virtual void proc_message_impl(DynamicMessage& dmsg) = 0;

    public: 
        uint16_t id; 
        bool hasInterrupt = false;
        bool isTrigger = false; 

        std::mutex m;
        std::condition_variable_any cv;
        bool processing = false;
        bool watchersPaused = false;  


        // Interrupt watch
        void addFileIWatch(std::string &fileName, std::function<bool()> handler = sendState){
            if(!hasInterrupt){
                hasInterrupt = true; 
            }
            this->Idesc_list.push_back({SrcType::UNIX_FILE, handler, fileName, 0}); 
        }

        void addGPIOIWatch(int gpio_port, std::function<bool()> handler = sendState){

            if(!hasInterrupt){
                hasInterrupt = true; 
            }
            this->Idesc_list.push_back({SrcType::GPIO, handler, "", gpio_port}); 
        }

        virtual void set_ports(std::unordered_map<std::string, std::string> &src) = 0;  

        virtual void proc_message(DynamicMessage input) {
            if (!hasInterrupt) {
                proc_message_impl(input);
                return;
            }
            // wait for interruptors to stop
            {
                std::unique_lock lk(m);
                cv.wait(lk, [this]{ return !watchersPaused; });
            }
            std::cout << "step 1: aquired lock from manageWatcher after unpausing" << std::endl;

            {
                std::lock_guard lk(m);
                processing = true;
            }
            std::cout << "step 2: processing to true, notify manageWatcher" << std::endl;
            cv.notify_one();

            {
                std::unique_lock lk(m);
                cv.wait(lk, [this]{ return watchersPaused; });
            }
            std::cout << "step 5: aquired lock from manageWatcher after pausing" << std::endl;
            
            // tell interruptors to start again
            {
                std::lock_guard lk(m);
                proc_message_impl(input);
                processing = false;
            }
            std::cout << "step 6: processing to false, notify manageWatcher" << std::endl;
            cv.notify_one();
        }

        virtual void read_data(DynamicMessage &newMsg) = 0; 
 
        virtual void close() {}; 

        virtual ~AbstractDevice() {}; 

        // Make the Interruptor a friend class: 
        friend class DeviceInterruptor; 

};



// Device Timer used for configuring polling rates dynamically; 

class DeviceTimer{
    private: 
        std::shared_ptr<AbstractDevice> device; 
        std::unordered_map<std::string, std::deque<float>> attr_history;  
        std::unordered_map<std::string, float> vol_map; 
        int ctl_code; 
        int device_code; 
        int timer_id; 
        // checks if the ticker is initialized
        bool ticker_init; 
        std::chrono::milliseconds period_time; 


        // Connection shard ptr
        std::shared_ptr<Connection> conex; 

        boost::asio::io_context &ctx; 
        boost::asio::steady_timer timer; 

        int poll_period = -1; 
        bool is_set = false; 

        std::chrono::milliseconds getRemainingTime(){
            auto now_time = std::chrono::steady_clock::now(); 
            auto timer_exp = timer.expires_at(); 
   
            if(now_time > timer_exp){
                auto difference = timer_exp - now_time; 
                return std::chrono::duration_cast<std::chrono::milliseconds>(difference);
            }   
            return std::chrono::milliseconds(0); 
        }



    public: 
        DeviceTimer(boost::asio::io_context &in_ctx, std::shared_ptr<AbstractDevice> abs, std::shared_ptr<Connection> cc, int ctl, int dev, int id)
        : ctx(in_ctx), timer(ctx), timer_id(id), ctl_code(ctl), device_code(dev)
        {
            device = abs; 
            conex = cc; 
        }

        void timerCallback(){
            this->timer.expires_after(this->period_time); 

            this->timer.async_wait([this](const boost::system::error_code &ec){
                if(!ec){
                    this->sendData(); 
                    timerCallback(); 
                }
            }); 
        }


        void setPeriod(int new_period){
            if(this->is_set == false){
                this->is_set = true; 
            }

            auto remTime = getRemainingTime(); 
            this->period_time = std::chrono::milliseconds(new_period); 

            // If new intialization or if the newtime is less than the remaining time
            if(this->period_time < remTime || this->poll_period == -1){

                this->timer.cancel();

                timerCallback(); 
                
                this->poll_period = new_period; 
            }

        }

        void Send(SentMessage &msg){
            
            this->conex->send(msg); 
        }

        float calculateStd(std::deque<float> &data){
            // Calculate the sum first: 
            float sum = 0; 
            float std = 0; 
            int size = data.size(); 

            for(auto& i : data){
                sum += i; 
            }
            float mean = sum/size; 

            for(auto& i : data){
                // Can never be negative as the difference is squared
                std += pow(i - mean, 2); 
            }

            return sqrt(std/size); 


        }


        void calcVolMap(){
            for(auto &pair : this->attr_history){
                this->vol_map[pair.first] = calculateStd(pair.second);
            }
        }

        void sendData(){
            SentMessage smsg; 
        
            DynamicMessage dmsg; 
            this->device->read_data(dmsg); 

            // Extract numerical data about the fields and add to the src: 
            dmsg.getFieldVolatility(this->attr_history, VOLATILITY_LIST_SIZE); 

            if(this->attr_history.size() > 0){
                calcVolMap(); 
                dmsg.createField("__DEV_ATTR_VOLATILITY__", this->vol_map); 
            }

            // Do some kind of data transformation here
            smsg.body = dmsg.Serialize(); 

            smsg.header.ctl_code = this->ctl_code; 
            smsg.header.device_code = this->device_code; 
            smsg.header.timer_id = this->timer_id; 
            smsg.header.prot = Protocol::SEND_STATE; 
            smsg.header.body_size = smsg.body.size(); 
            smsg.header.fromInterrupt = false; 

            Send(smsg); 
        }

        ~DeviceTimer() {
            this->timer.cancel();
        }

}; 

/* 
    INTERRUPT STUFF
*/



class DeviceInterruptor{
    private: 
        std::shared_ptr<AbstractDevice> abs_device; 
        std::shared_ptr<Connection> client_connection; 
        std::jthread watcherManagerThread;
        std::vector<std::jthread> globalWatcherThreads; 
        std::vector<std::tuple<int, int, std::string>> watchDescriptors;
        int ctl_code; 
        int device_code; 


        void sendMessage(){
            // Configure sent message header: 
            SentMessage sm; 
            sm.header.ctl_code = this->ctl_code; 
            sm.header.device_code = this->device_code; 
            sm.header.prot = Protocol::SEND_STATE; 
            sm.header.fromInterrupt = true; 

            // Change the timer_id specification: 
            sm.header.timer_id = -1;

            // Volatility does not need to be recorded
            sm.header.volatility = 0; 
        
            DynamicMessage dmsg; 
            this->abs_device->read_data(dmsg); 
            sm.body = dmsg.Serialize(); 

            sm.header.body_size = sm.body.size() ; 

            this->client_connection->send(sm); 
        }

        void disableWatchers() { 
            for (auto&& [fd, wd, filename] : watchDescriptors) {
                inotify_rm_watch(fd, wd);
            }
        }

        void enableWatchers() {
            for (auto& [fd, wd, filename] : watchDescriptors) {
                wd = inotify_add_watch(fd, filename.c_str(), IN_CLOSE_WRITE); 
                if(wd < 0){
                    std::cerr<<"Could not add watcher"<<std::endl; 
                    close(fd);
                }
            }
        }

        void manageWatchers(std::stop_token stoken) {
            auto& m = this->abs_device->m;
            auto& cv = this->abs_device->cv;
            auto& processing = this->abs_device->processing;
            auto& watchersPaused = this->abs_device->watchersPaused;
            while (!stoken.stop_requested()) {
                // wait for device to receive a new message to process or for device reset
                {
                    std::unique_lock lk(m);
                    if (!cv.wait(lk, stoken, [&processing] { return processing; })) {
                        break; // break early to avoid unnecessary lock aquisition overhead
                    }
                }

                // disable watchers before message processing
                {
                    std::lock_guard lk(m);
                    disableWatchers();
                    watchersPaused = true;
                }
                cv.notify_one();

                // wait for message to process
                {
                    std::unique_lock lk(m);
                    cv.wait(lk, [&processing] { return !processing; });
                }

                // re-enable watchers
                {
                    std::lock_guard lk(m);
                    enableWatchers();
                    watchersPaused = false;
                }
                cv.notify_one();
            }
            disableWatchers();
        }

        // Add Inotify thread blocking code heres
        void IFileWatcher(std::stop_token stoken, std::string fname, std::function<bool()> handler ){
            // Check if the file exists relative to the deviceCore; 
            
            int fd = inotify_init(); 
            if(fd < 0){
                std::cerr<<"Could not make Inotify object"<<std::endl; 
                close(fd); 
            }

            // The IFileWatcher makes the call when the file is modified
            int wd = inotify_add_watch(fd, fname.c_str(), IN_CLOSE_WRITE); 
            if(wd < 0){
                std::cerr<<"Could not perform an inotify event"<<std::endl; 
                close(fd); 
            }

            watchDescriptors.push_back({fd, wd, fname});

            // For now we can bypass the metadata and store data for the filesize and stuff;
            char event_buffer[sizeof(inotify_event) + 256]; 
            while(!stoken.stop_requested()){
                std::cout<<"Waiting for event"<<std::endl;
                int read_length = read(fd, event_buffer, sizeof(event_buffer)); 
                std::cout<<"File Written to"<<fname<<std::endl; 

                struct inotify_event* event = (struct inotify_event*)event_buffer; 
                if (event->mask == IN_IGNORED) {
                    std::cout << "drop removed watch event" << std::endl;
                    continue;
                }

                bool ret_val = handler(); 
                if(ret_val && !this->abs_device->processing){
                    std::cerr<<"Sending message"<<std::endl;
                    this->sendMessage();  
                }
            }
        }

        void IGpioWatcher(std::stop_token stoken, int portNum, std::function<bool()> handler){
                std::cerr<<"GPIO Interrupts not yet supported!"<<std::endl; 
        }
        
    public: 
        // Device Interruptor
        DeviceInterruptor(std::shared_ptr<AbstractDevice> targDev,  std::shared_ptr<Connection> conex,  
        int ctl, int dd)
        {
            this->client_connection = conex; 
            this->abs_device = targDev; 
            this->ctl_code = ctl; 
            this->device_code = dd; 
        }

        // Setup Watcher Thread
        void setupThreads(){
            // Create the threads
            auto iFileWatcher = std::bind(&DeviceInterruptor::IFileWatcher, std::ref(*this), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
            auto iGpioWatcher = std::bind(&DeviceInterruptor::IGpioWatcher, std::ref(*this), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
            auto manageWatchers = std::bind(&DeviceInterruptor::manageWatchers, std::ref(*this), std::placeholders::_1);
            for(auto& idesc : this->abs_device->Idesc_list){
                switch(idesc.src_type){
                    case(SrcType::UNIX_FILE) : {
                        this->globalWatcherThreads.emplace_back(iFileWatcher, idesc.file_src, idesc.handler); 
                        break; 
                    }

                    case(SrcType::GPIO): {
                        this->globalWatcherThreads.emplace_back(iGpioWatcher, idesc.port_num, idesc.handler);
                        break; 
                    }

                    default :{
                        std::cout<<"Unimplemented interrupt watcher"<<std::endl; 
                    }
                }
            }
            watcherManagerThread = std::jthread(manageWatchers);
        }

        ~DeviceInterruptor() {
            for (auto&& watcher : globalWatcherThreads) {
                watcher.request_stop();
            }
            watcherManagerThread.request_stop();
        }
}; 


