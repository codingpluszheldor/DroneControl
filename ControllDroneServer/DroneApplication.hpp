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
            std::cerr << "������ ��������: " << nn_strerror(nn_errno()) << "\n";
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
    /// ������������� ������� rpc �� nanomsg
    /// </summary>
    /// <param name="endpoint">����� ��� ����������� ��������</param>
    /// <return>��������� �������� ������</return>
    int initRpcControllServer(const std::string& endpoint)
    {
        // ����� nanomsg
        _server_sock = nn_socket(AF_SP, NN_REP);
        if (_server_sock < 0) {
            std::cerr << "������ ������������� socket\n";
            return -1;
        }

        if (nn_bind(_server_sock, endpoint.c_str()) < 0) {
            std::cerr << "������ �������� ������: " << endpoint << "\n";
            nn_close(_server_sock);
            return -1;
        }

        return 0;
    }

    /// <summary>
    /// ���������
    /// </summary>
    void airSimParams(DroneMethodReq* request)
    {
        _client._speed = request->speed;
        _client._drivetrain = static_cast<DrivetrainType>(request->drivetrain);
        _client._yaw_is_rate = request->yaw_is_rate;
        _client._yaw_or_rate = request->yaw_or_rate;
    }

    /// <summary>
    /// �������� ���� ���������
    /// </summary>
    void airSimApi(const DroneMethods& method)
    {
        try {
            switch (method) {
            case DroneMethods::BarometerData: {
                BarometerBase::Output barometer_data = std::move(_client.barometerData());
                DroneReply* reply = new DroneReply;
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
                makeResponseControl(DroneMethods::Connection);
                break;
            }
            case DroneMethods::Takeoff: {
                _client.takeoff();
                makeResponseControl(DroneMethods::Takeoff);
                break;
            }
            case DroneMethods::TestFlyBox: {
                _client.testFlyBox();
                makeResponseControl(DroneMethods::TestFlyBox);
                break;
            }
            case DroneMethods::Landing: {
                _client.landing();
                makeResponseControl(DroneMethods::Landing);
                break;
            }
            case DroneMethods::Arm: {
                _client.armDisarm();
                makeResponseControl(DroneMethods::Arm);
                break;
            }
            case DroneMethods::Disarm: {
                _client.armDisarm(false);
                makeResponseControl(DroneMethods::Disarm);
                break;
            }
            case DroneMethods::ToUp: {
                _client.toUpFly();
                makeResponseControl(DroneMethods::ToUp);
                break;
            }
            case DroneMethods::ToDown: {
                _client.toDownFly();
                makeResponseControl(DroneMethods::ToDown);
                break;
            }
            case DroneMethods::ToForward: {
                _client.toForwardFly();
                makeResponseControl(DroneMethods::ToForward);
                break;
            }
            case DroneMethods::ToRight: {
                _client.toRightFly();
                makeResponseControl(DroneMethods::ToRight);
                break;
            }
            case DroneMethods::ToLeft: {
                _client.toLeftFly();
                makeResponseControl(DroneMethods::ToLeft);
                break;
            }
            case DroneMethods::ToBack: {
                _client.toBackFly();
                makeResponseControl(DroneMethods::ToBack);
                break;
            }
            }
        }
        catch (rpc::rpc_error& e) {
            const auto msg = e.get_error().as<std::string>();
            std::cout << "Exception raised by the API, something went wrong." << std::endl
                      << msg << std::endl;
        }
    }

    /// <summary>
    /// ������ ����� ���������
    /// </summary>
    int run()
    {
        std::thread receiver_thread(receive_messages, _server_sock, std::ref(_incoming_queue));
        std::thread sender_thread(send_responses, _server_sock, std::ref(_outgoing_queue));

        try {
            while (true) {
                // �������� ��������� �� �������
                std::vector<std::byte> incoming_message = _incoming_queue.pop();
                std::cout << "�������� ���������, ������: " << incoming_message.size() << '\n';

                _response.clear();
                DroneMethodReq *request = reinterpret_cast<DroneMethodReq*>(incoming_message.data());
                if (request != nullptr) {
                    airSimParams(request);
                    airSimApi(request->method);
                } else {
                    continue;
                }
                
                if (_response.empty()) {
                    continue;
                }
                // �����
                std::cout << "��������, ������: " << _response.size() << std::endl;
                _outgoing_queue.push(std::move(_response)); 
            }
        }
        catch (...) {
            std::cerr << "���-�� ����� �� ���...\n";
        }

        receiver_thread.join();
        sender_thread.join();
        nn_shutdown(_server_sock, 0);
        nn_close(_server_sock);
        return 0;
    }

private:
    /// <summary>
    /// �������� ��������� ������ �� ���������� �������
    /// </summary>
    void makeResponseControl(const DroneMethods method)
    {
        DroneReply* reply = new DroneReply;
        reply->method = method;
        _response = std::vector<std::byte>(reinterpret_cast<const std::byte*>(reply),
                                           reinterpret_cast<const std::byte*>(reply + sizeof(DroneReply)));
        delete reply;
    }
};

}

#endif