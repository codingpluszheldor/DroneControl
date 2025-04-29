#ifndef DRONE_APPLICATION_HPP
#define DRONE_APPLICATION_HPP

#include "common/common_utils/StrictMode.hpp"
STRICT_MODE_OFF
#ifndef RPCLIB_MSGPACK
#define RPCLIB_MSGPACK clmdep_msgpack
#endif // !RPCLIB_MSGPACK
#include "rpc/rpc_error.h"
STRICT_MODE_ON

#include "vehicles/multirotor/api/MultirotorRpcLibClient.hpp"
#include "common/common_utils/FileSystem.hpp"
#include <iostream>
#include <chrono>
#include <string>
#include <vector>

#include <compat/nanomsg/nn.h>
#include <compat/nanomsg/reqrep.h>

#include "DroneAirSimClient.hpp"
#include "DroneRpc.hpp"
#include "SafeMessageQueue.hpp"

using namespace msr::airlib;

namespace drone
{
constexpr std::uint32_t MSG_BUFFER_SIZE = 1024 * 10;

void receive_messages(int sock_fd, SafeMessageQueue<std::vector<std::byte>> &incoming_queue)
{
    while (true) {
        char buffer[MSG_BUFFER_SIZE];
        int bytes_received = nn_recv(sock_fd, buffer, sizeof(buffer), 0);
        if (bytes_received >= 0) {
            incoming_queue.push(std::vector<std::byte>(reinterpret_cast<const std::byte*>(buffer),
                                                       reinterpret_cast<const std::byte*>(buffer + sizeof(buffer))));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void send_responses(int sock_fd, SafeMessageQueue<std::vector<std::byte>> &outgoing_queue)
{
    while (true) {
        std::vector<std::byte> response = outgoing_queue.pop();
        if (nn_send(sock_fd, reinterpret_cast<char*>(response.data()), response.size(), 0) < 0) { 
            std::cerr << "Ошибка отправки: " << nn_strerror(nn_errno()) << "\n";
        }
    }
}

class DroneApplication 
{
private:
    DroneAirSimClient _client;
    SafeMessageQueue<std::vector<std::byte>> _incoming_queue;
    SafeMessageQueue<std::vector<std::byte>> _outgoing_queue;
    std::vector<std::byte> _response;
    int _server_sock = -1;

public:
    /// <summary>
    /// Инициализация сервера rpc на nanomsg
    /// </summary>
    /// <param name="endpoint">Адрес для подключения клиентов</param>
    /// <return>Результат биндинга сокета</return>
    int initRpcControllServer(const std::string& endpoint)
    {
        // Сокет nanomsg
        _server_sock = nn_socket(AF_SP, NN_REP);
        if (_server_sock < 0) {
            std::cerr << "Ошибка инициализации socket\n";
            return -1;
        }

        if (nn_bind(_server_sock, "tcp://127.0.0.1:20001") < 0) {
            std::cerr << "Ошибка привязки адреса: " << endpoint << "\n";
            nn_close(_server_sock);
            return -1;
        }

        return 0;
    }

    /// <summary>
    /// Основной цикл программы
    /// </summary>
    void airSimApi(const DroneMethods& method)
    {
        try {
            switch (method) {
                case DroneMethods::BarometerData: {
                    BarometerBase::Output barometer_data = std::move(_client.barometerData());
                    DroneReply *reply = new DroneReply;
                    reply->method = DroneMethods::BarometerData;
                    reply->barometer = {
                        barometer_data.time_stamp,
                        barometer_data.altitude,
                        barometer_data.pressure,
                        barometer_data.qnh
                    };
                    _response = std::vector<std::byte>(reinterpret_cast<const std::byte*>(reply),
                                                       reinterpret_cast<const std::byte*>(reply + sizeof(DroneReply)));
                    delete reply;
                    break;
                } 
                case DroneMethods::Connection: {
                    _client.connection();
                    _client.armDisarm();
                    DroneReply* reply = new DroneReply;
                    reply->method = DroneMethods::Connection;
                    _response = std::vector<std::byte>(reinterpret_cast<const std::byte*>(reply),
                                                       reinterpret_cast<const std::byte*>(reply + sizeof(DroneReply)));
                    delete reply;
                    break;
                }
                case DroneMethods::Takeoff:
                    _client.takeoff();
                    break;
                case DroneMethods::TestFlyBox:
                    _client.testFlyBox();
                    break;
                case DroneMethods::Landing:
                    _client.landing();
                    break;
                case DroneMethods::Arm:
                    _client.armDisarm();
                    break;
                case DroneMethods::Disarm:
                    _client.armDisarm(false);
                    break;
            }
        }
        catch (rpc::rpc_error& e) {
            const auto msg = e.get_error().as<std::string>();
            std::cout << "Exception raised by the API, something went wrong." << std::endl
                      << msg << std::endl;
        }
    }

    /// <summary>
    /// Запуск цикла сообщений
    /// </summary>
    int run()
    {
        std::thread receiver_thread(receive_messages, _server_sock, std::ref(_incoming_queue));
        std::thread sender_thread(send_responses, _server_sock, std::ref(_outgoing_queue));

        try {
            while (true) {
                // Ожидание сообщения от клиента
                std::vector<std::byte> incoming_message = _incoming_queue.pop();
                std::cout << "Получено сообщение, размер: " << incoming_message.size() << '\n';

                _response.clear();
                DroneMethodReq *request = reinterpret_cast<DroneMethodReq*>(incoming_message.data());
                if (request != nullptr) {
                    airSimApi(request->method);
                } else {
                    continue;
                }
                
                if (_response.empty()) {
                    continue;
                }
                // Ответ
                std::cout << "Отправка, размер: " << _response.size() << std::endl;
                _outgoing_queue.push(std::move(_response)); 
            }
        }
        catch (...) {
            std::cerr << "Что-то пошло не так...\n";
        }

        receiver_thread.join();
        sender_thread.join();
        nn_shutdown(_server_sock, 0);
        nn_close(_server_sock);
        return 0;
    }
};

}

#endif