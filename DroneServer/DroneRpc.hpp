#ifndef DRONE_RPC_HPP
#define DRONE_RPC_HPP

#include <cinttypes>
#include <cmath>

/// <summary>
/// ������� �� ����� �����
/// </summary>
enum class DroneSensors
{
    Barometer,
    Imu,
    Gps,
    Magnetometer
};

/// <summary>
/// ������ ������� ��� ���������� � ��������� ����������
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
/// ������ �� ���������� �������
/// </summary>
struct DroneMethodReq
{
    DroneMethods method;
    std::uint64_t time_point;
};

/// <summary>
/// ��������� ������ �� ������� ��������
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