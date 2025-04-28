#ifndef DRONE_RPC_HPP
#define DRONE_RPC_HPP

#include <cinttypes>
#include <cmath>

/// <summary>
/// Сенсоры на борту дрона
/// </summary>
enum class DroneSensors
{
    Barometer,
    Imu,
    Gps,
    Magnetometer
};

/// <summary>
/// Список комманд для упарвления и получения телеметрии
/// </summary>
enum class DroneMethods
{
    Connection,
    Arm,
    Disarm,
    Takeoff,
    Landing,
    TestFlyBox,
    BarometerData,
    ImuData,
    GpsData,
    MagnetometerData
};

/// <summary>
/// Запрос на выполнение команды
/// </summary>
struct DroneMethodReq
{
    DroneMethods method;
    std::uint64_t time_point;
};

/// <summary>
/// Структура ответа от сенсора давления
/// </summary>
struct BarometerSensorDataRep
{
    DroneSensors sensor;
    std::uint64_t time_point;
    std::float_t altitude; //meters
    std::float_t pressure; //Pascal
    std::float_t qnh;
};


#endif