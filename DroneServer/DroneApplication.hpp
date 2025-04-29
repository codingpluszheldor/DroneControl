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

#include <compat/nanomsg/nn.h>
#include <compat/nanomsg/reqrep.h>

#include "DroneAirSimClient.hpp"
#include "DroneRpc.hpp"

using namespace msr::airlib;

namespace drone
{
class DroneApplication 
{
private:
    DroneAirSimClient _client;

public:
    /// <summary>
    /// Инициализация сервера rpc на nanomsg
    /// </summary>
    /// <param name="endpoint">Адрес для подключения клиентов</param>
    /// <return>Результат биндинга сокета</return>
    int initRpcControllServer(const std::string& endpoint)
    {
        // Сокет nanomsg
        int server_sock = nn_socket(AF_SP, NN_REP);
        if (server_sock < 0) {
            std::cerr << "Ошибка инициализации socket\n";
            return -1;
        }

        if (nn_bind(server_sock, endpoint.c_str()) < 0) {
            std::cerr << "Ошибка привязки адреса\n";
            nn_close(server_sock);
            return -1;
        }

         return 0;
    }

    /// <summary>
    /// Основной цикл программы
    /// </summary>
    int run()
    {
        try {
            std::this_thread::sleep_for(std::chrono::duration<double>(5.0));
            _client.connection();
            _client.armDisarm();
            std::this_thread::sleep_for(std::chrono::duration<double>(5.0));
            _client.takeoff();

            std::this_thread::sleep_for(std::chrono::duration<double>(5.0));
            BarometerBase::Output barometer_data = std::move(_client.barometerData());
            std::cout << "Barometer data \n"
                      << "barometer_data.time_stamp \t" << barometer_data.time_stamp << std::endl
                      << "barometer_data.altitude \t" << barometer_data.altitude << std::endl
                      << "barometer_data.pressure \t" << barometer_data.pressure << std::endl
                      << "barometer_data.qnh \t" << barometer_data.qnh << std::endl;
            BarometerSensorDataRep bar_sensor{
                DroneSensors::Barometer,
                barometer_data.time_stamp,
                barometer_data.altitude,
                barometer_data.pressure,
                barometer_data.qnh
            };

            _client.testFlyBox();
            std::this_thread::sleep_for(std::chrono::duration<double>(5.0));
            _client.landing();
            std::this_thread::sleep_for(std::chrono::duration<double>(5.0));
            _client.armDisarm(false);
        } catch (rpc::rpc_error& e) {
            const auto msg = e.get_error().as<std::string>();
            std::cout << "Exception raised by the API, something went wrong." << std::endl
                      << msg << std::endl;
        }

        return 0;
    }
};

}

#endif