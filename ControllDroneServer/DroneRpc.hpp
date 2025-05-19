#ifndef DRONE_RPC_HPP
#define DRONE_RPC_HPP

#include <cinttypes>
#include <cmath>
#include <string>
#include <map>


namespace drone
{
constexpr std::uint32_t MSG_BUFFER_SIZE = 1024 * 20;

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
enum class DroneMethods : int
{
    // Ожидание команды
    Wait = 0,
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
    ToBack,
    RotateLeft,
    RotateRight
};

/// <summary>
/// Список камер дрона
/// </summary>
enum class DroneCamera : int
{
    front_center = 0,
    front_right,
    front_left,
    fpv,
    back_center
};

static std::map<DroneCamera, std::string> map_cameras = {
    { DroneCamera::front_center, "front-center" },
    { DroneCamera::front_right,  "front-right"  },
    { DroneCamera::front_left,   "front-left"   },
    { DroneCamera::fpv,          "fpv"          },
    { DroneCamera::back_center,  "back-center"  }
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
    bool get_camera_image = false;
    DroneCamera camera = DroneCamera::front_center;
};
#pragma pack(pop)

/// <summary>
/// Структура ответа от сенсора давления
/// </summary>
#pragma pack(push, 1)
struct BarometerSensorDataRep
{
    std::uint64_t time_point = 0;
    std::float_t altitude = 0.0f; //meters
    std::float_t pressure = 0.0f; //Pascal
    std::float_t qnh = 0.0f;
};
#pragma pack(pop)

/// <summary>
/// Структура ответа от ИНС
/// </summary>
#pragma pack(push, 1)
struct ImuSensorDataRep
{
    std::uint64_t time_point = 0;
    std::float_t angular_velocity_x = 0.0f;
    std::float_t angular_velocity_y = 0.0f;
    std::float_t angular_velocity_z = 0.0f;
    std::float_t linear_acceleration_x = 0.0f;
    std::float_t linear_acceleration_y = 0.0f;
    std::float_t linear_acceleration_z = 0.0f;
};
#pragma pack(pop)

/// <summary>
/// Структура ответа от ИНС
/// </summary>
#pragma pack(push, 1)
struct GpsSensorDataRep
{
    std::uint64_t time_point = 0;
    std::double_t latitude = 0.0;
    std::double_t longitude = 0.0;
    std::float_t  altitude = 0.0f;
    std::float_t velocity_x = 0.0f;
    std::float_t velocity_y = 0.0f;
    std::float_t velocity_z = 0.0f;
    //GPS HDOP/VDOP horizontal/vertical dilution of position (unitless), 0-100%
    std::float_t eph;
    std::float_t epv;
    bool is_valid = false;
};
#pragma pack(pop)

/// <summary>
/// Структура ответа от магнитометра
/// </summary>
#pragma pack(push, 1)
struct MagnetometerSensorDataRep
{
    std::uint64_t time_point = 0;
    std::float_t x = 0.0f;
    std::float_t y = 0.0f;
    std::float_t z = 0.0f;
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
    ImuSensorDataRep imu;
    GpsSensorDataRep gps;
    MagnetometerSensorDataRep magnetometer;
};
#pragma pack(pop)

}

#endif
