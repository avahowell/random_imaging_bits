
#ifndef THREADQUEUE_H
#define THREADQUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>


// threadqueue class: thread-safe FIFO queue
template<typename T>
class threadqueue {
private:
    std::mutex m;
    std::condition_variable c;
    std::queue<T> q;
public:
    T pop()
    {
        std::unique_lock<std::mutex> lock(m);
        while (q.empty())
        {
            c.wait(lock);
        }
        T item = q.front();
        q.pop();
        return item;
    }
    void push(const T& item)
    {
        std::unique_lock<std::mutex> lock(m);
        q.push(item);
        lock.unlock();
        c.notify_one();
    }
    void clear()
    {
        q = std::queue<T>();
    }
};


#endif // THREADQUEUE_H
