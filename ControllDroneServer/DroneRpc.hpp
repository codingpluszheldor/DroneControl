#ifndef DRONE_RPC_HPP
#define DRONE_RPC_HPP

#include <cinttypes>
#include <cmath>


namespace drone
{
constexpr std::uint32_t MSG_BUFFER_SIZE = 1024 * 10;

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
    // Ожидание команды
    Wait,
    // Соединения и проверка
    Connection,
    Arm,
    Disarm,
    Takeoff,
    Landing,
    TestFlyBox,
    // Сенсоры
    BarometerData,
    ImuData,
    GpsData,
    MagnetometerData,
    // Пульт управления
    ToUp,
    ToDown,
    ToLeft,
    ToRight,
    ToForward,
    ToBack
};

/// <summary>
/// Запрос на выполнение команды
/// </summary>
#pragma pack(push, 1)
struct DroneMethodReq
{
    DroneMethods method;
    std::uint64_t time_point = 0;
    bool yaw_is_rate = true;
    float yaw_or_rate = 0.0f; // угол рысканья
    float speed = 5.0f; // скорость
    int drivetrain = 1; // DrivetrainType::ForwardOnly;
};
#pragma pack(pop)

/// <summary>
/// Структура ответа от сенсора давления
/// </summary>
#pragma pack(push, 1)
struct BarometerSensorDataRep
{
    std::uint64_t time_point;
    std::float_t altitude; //meters
    std::float_t pressure; //Pascal
    std::float_t qnh;
};
#pragma pack(pop)

/// <summary>
/// Структура общего ответа
/// </summary>
#pragma pack(push, 1)
struct DroneReply
{
    DroneMethods method;
    BarometerSensorDataRep barometer;
};
#pragma pack(pop)

}

#endif
