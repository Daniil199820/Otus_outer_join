#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include "Ditchr_Requests.h"
#include <chrono>

class Client_require{
public:
    using TTask = std::function<void()>;
	using TCallback = std::function<void()>;

    Client_require():m_stopped{false}{
        req_mngr = std::make_unique<Request_manager>();
        worker_thread = std::thread(&Client_require::process,this);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }


    void make_request(const std::string& command_str){
        {
            std::lock_guard<std::mutex> guard(worker_mutex);
            m_tasks.push(command_str);
        }
        wait_worker.notify_all();
    }

    ~Client_require(){
        m_stopped = true;
        wait_worker.notify_all();
        if(worker_thread.joinable()){
            worker_thread.join();
        }
    }


private:
    std::thread worker_thread;
    std::atomic<bool> m_stopped;
    std::mutex worker_mutex;
    std::condition_variable wait_worker;
    std::queue<std::string> m_tasks;
    std::unique_ptr<Request_manager>req_mngr;

    void process(){
        while(m_stopped==false){
            std::unique_lock<std::mutex> locker(worker_mutex);
            wait_worker.wait(locker,[&](){return !m_tasks.empty()||m_stopped==true;});
            while(!m_tasks.empty()){
                std::string temp_command = m_tasks.front();
                m_tasks.pop();
                req_mngr.get()->set_request(temp_command);
            }
        }   
    }
};