#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>

using namespace std::chrono_literals;

template<typename T>
class safe_queue
{
public:
    //метод push – записывает в начало очереди новую задачу.
    void push(T value)
    {
        std::unique_lock<std::mutex> lock(mutex);
        queue.push(std::move(value));
        lock.unlock();
        cond.notify_one();
    }
    //метод queue_pop проверяет если в очереди есть не исполненные задачи, 
    //если есть отправляет на исполнение и удаляет элемент из очереди
    bool queue_pop(T& value)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (queue.empty())
        {
            return false;
        }
        else {
            value = std::move(queue.front());
            queue.pop();
            return true;
        }

    }
private:
    std::queue<T> queue;//очередь для хранения задач
    mutable std::mutex mutex; //мьютекс для блокировки
    std::condition_variable cond;//std::condtional_variables для уведомлений
};


class thread_pool
{
public:
    thread_pool() //Конструктор thread_pool
    {
        for (int i = 0; i < v_size; i++)
        {
            v_threads.push_back(std::thread(&thread_pool::work, this));
        }
    }

    ~thread_pool() //Деструктор thread_pool 
    {
        job_done = true;
        for (auto& th : v_threads)
        {
            th.join();
        }
    }
    //метод work – выбирает из очереди очередную задачу и исполняет ее.
    void work()
    {
        while (!job_done)
        {
            std::function<void()> func;
            if (queue.queue_pop(func))
            {
                func();
            }
            else {
                std::this_thread::yield();
            }
        }
    }
    //метод submit – помещает в очередь очередную задачу. 
    template<typename Func>
    void submit(Func&& f)
    {
        queue.push(std::forward<Func>(f));
    }

private:
    int v_size = std::thread::hardware_concurrency();//Размер вектора потоков
    std::vector<std::thread> v_threads;//вектор потоков
    safe_queue<std::function<void()>> queue;//потокобезопасная очередь задач
    std::atomic<bool> job_done{false};//Флаг для передачи потоку
};

void function_test1()
{
    std::this_thread::sleep_for(200ms);
    std::cout << __FUNCTION__ << std::endl;
}

void function_test2()
{
    std::this_thread::sleep_for(500ms);
    std::cout << __FUNCTION__ << std::endl;
}

int main() {
    thread_pool t_pool;

    for (int i = 0; i < 10; ++i)
    {
        t_pool.submit(function_test1);
        t_pool.submit(function_test2);
        std::this_thread::sleep_for(1s);
    }

    return 0;
}
