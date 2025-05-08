#ifndef SAFE_MSG_QUEUE_HPP
#define SAFE_MSG_QUEUE_HPP

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <string>

namespace drone
{
/// <summary>
/// �������-���������� ������� ���������
/// </summary>
template <typename MessageType>
class SafeMessageQueue
{
private:
    std::mutex _mtx;
    std::condition_variable _cond_var;
    std::queue<MessageType> _messages;

public:
    /// <summary>
    /// ���������� ��������� � �������
    /// </summary>
    template <typename T>
    void push(T&& message)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        _messages.emplace(std::forward<T>(message));
        _cond_var.notify_one();
    }

    /// <summary>
    /// �������� ��������� �� �������
    /// </summary>
    MessageType pop()
    {
        std::unique_lock<std::mutex> lock(_mtx);
        _cond_var.wait(lock, [this]() { return !_messages.empty(); });
        auto result = _messages.front();
        _messages.pop();
        return result;
    }
};
}

#endif