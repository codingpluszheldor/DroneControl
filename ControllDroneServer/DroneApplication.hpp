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

        if (nn_bind(_server_sock, endpoint.c_str()) < 0) {
            std::cerr << "Ошибка привязки адреса: " << endpoint << "\n";
            nn_close(_server_sock);
            return -1;
        }

        return 0;
    }

    /// <summary>
    /// Параметры
    /// </summary>
    void airSimParams(DroneMethodReq* request)
    {
        _client._speed = request->speed;
        _client._drivetrain = static_cast<DrivetrainType>(request->drivetrain);
        _client._yaw_is_rate = request->yaw_is_rate;
        _client._yaw_or_rate = request->yaw_or_rate;
    }

    /// <summary>
    /// Основной цикл программы
    /// </summary>
    void airSimApi(const DroneMethods& method, const bool get_image = false)
    {
        try {
            switch (method) {
            case DroneMethods::Connection: {
                _client.connection();
                makeResponseControl(DroneMethods::Connection, get_image);
                break;
            }
            case DroneMethods::Takeoff: {
                _client.takeoff();
                makeResponseControl(DroneMethods::Takeoff, get_image);
                break;
            }
            case DroneMethods::TestFlyBox: {
                _client.testFlyBox();
                makeResponseControl(DroneMethods::TestFlyBox, get_image);
                break;
            }
            case DroneMethods::Landing: {
                _client.landing();
                makeResponseControl(DroneMethods::Landing, get_image);
                break;
            }
            case DroneMethods::Arm: {
                _client.armDisarm();
                makeResponseControl(DroneMethods::Arm, get_image);
                break;
            }
            case DroneMethods::Disarm: {
                _client.armDisarm(false);
                makeResponseControl(DroneMethods::Disarm, get_image);
                break;
            }
            case DroneMethods::ToUp: {
                _client.toUpFly();
                makeResponseControl(DroneMethods::ToUp, get_image);
                break;
            }
            case DroneMethods::ToDown: {
                _client.toDownFly();
                makeResponseControl(DroneMethods::ToDown, get_image);
                break;
            }
            case DroneMethods::ToForward: {
                _client.toForwardFly();
                makeResponseControl(DroneMethods::ToForward, get_image);
                break;
            }
            case DroneMethods::ToRight: {
                _client.toRightFly();
                makeResponseControl(DroneMethods::ToRight, get_image);
                break;
            }
            case DroneMethods::ToLeft: {
                _client.toLeftFly();
                makeResponseControl(DroneMethods::ToLeft, get_image);
                break;
            }
            case DroneMethods::ToBack: {
                _client.toBackFly();
                makeResponseControl(DroneMethods::ToBack, get_image);
                break;
            }
            case DroneMethods::RotateLeft: {
                _client.rotateByYaw();
                makeResponseControl(DroneMethods::RotateLeft, get_image);
                break;
            }
            case DroneMethods::RotateRight: {
                _client.rotateByYaw(false);
                makeResponseControl(DroneMethods::RotateRight, get_image);
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
                    airSimParams(request);
                    airSimApi(request->method, request->get_camera_image);
                } else {
                    continue;
                }
                
                if (_response.empty()) {
                    std::cerr << "Пустой ответ...\n";
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

private:
    /// <summary>
    /// Создание структуры ответа на упраляющую команду
    /// </summary>
    void makeResponseControl(const DroneMethods method, const bool get_image = false)
    {
        DroneReply* reply = new DroneReply;
        reply->method = method;

        if (method != DroneMethods::Connection) {
            BarometerBase::Output &&barometer_data = _client.barometerData();
            reply->barometer = {
                barometer_data.time_stamp,
                barometer_data.altitude,
                barometer_data.pressure,
                barometer_data.qnh
            };

            ImuBase::Output &&imu_data = _client.imuData();
            reply->imu = {
                imu_data.time_stamp,
                imu_data.angular_velocity.x(),
                imu_data.angular_velocity.y(),
                imu_data.angular_velocity.z(),
                imu_data.linear_acceleration.x(),
                imu_data.linear_acceleration.y(),
                imu_data.linear_acceleration.z(),
            };

            GpsBase::Output &&gps_data = _client.gpsData();
            reply->gps = {
                gps_data.time_stamp,
                gps_data.gnss.geo_point.latitude,
                gps_data.gnss.geo_point.longitude,
                gps_data.gnss.geo_point.altitude,
                gps_data.gnss.velocity.x(),
                gps_data.gnss.velocity.y(),
                gps_data.gnss.velocity.z(),
                gps_data.gnss.eph,
                gps_data.gnss.epv,
                gps_data.is_valid
            };

            MagnetometerBase::Output&& magnetometer_data = _client.magnetometerData();
            reply->magnetometer = {
                magnetometer_data.time_stamp,
                magnetometer_data.magnetic_field_body.x(),
                magnetometer_data.magnetic_field_body.y(),
                magnetometer_data.magnetic_field_body.z()
            };  


            if (get_image) {
                const std::vector<ImageResponse> img_response = _client.cameraImage();
                // VAS: test to files
                for (const ImageResponse& image_info : img_response) {
                    std::string path = "D:\\Documents\\AirSim\\Recordings";
                    std::string file_path = FileSystem::combine(path, std::to_string(image_info.time_stamp));
                    std::ofstream file(file_path + ".png", std::ios::binary);
                    file.write(reinterpret_cast<const char*>(image_info.image_data_uint8.data()), image_info.image_data_uint8.size());
                    file.close();
                }
            }
        }
        
        _response = std::vector<std::byte>(reinterpret_cast<const std::byte*>(reply),
                                           reinterpret_cast<const std::byte*>(reply + sizeof(DroneReply)));
        delete reply;
    }
};

}

#endif