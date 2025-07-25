#ifndef DRONE_AIRSIM_CLIENT_HPP
#define DRONE_AIRSIM_CLIENT_HPP

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
#include <math.h>

using namespace msr::airlib;

typedef ImageCaptureBase::ImageRequest ImageRequest;
typedef ImageCaptureBase::ImageResponse ImageResponse;
typedef ImageCaptureBase::ImageType ImageType;
typedef common_utils::FileSystem FileSystem;

namespace drone
{
/// <summary>
/// AirSim клиент
/// </summary>
class DroneAirSimClient
{
private:
    MultirotorRpcLibClient _client;

public:
    bool _yaw_is_rate = false;
    float _yaw_or_rate = 0.0f; // угол рысканья
    float _speed = 5.0f; // скорость    
    DrivetrainType _drivetrain = DrivetrainType::ForwardOnly; // наклоном вперёд

public:
    /// <summary>
    /// Соединение с симулятором
    /// </summary>
    void connection()
    {
        _client.confirmConnection();
        _client.enableApiControl(true);
    }

    /// <summary>
    /// Включение/выключение дрона
    /// </summary>
    void armDisarm(bool arm = true)
    {
        _client.enableApiControl(true);
        _client.armDisarm(arm);
    }

    /// <summary>
    /// Взлёт
    /// </summary>
    void takeoff(const float takeoff_timeout = 5)
    {
        _client.enableApiControl(true);
        _client.takeoffAsync(takeoff_timeout)->waitOnLastTask();
        // switch to explicit hover mode so that this is the fall back when
        // move* commands are finished.
        std::this_thread::sleep_for(std::chrono::duration<double>(5));
        _client.hoverAsync()->waitOnLastTask();
    }

    /// <summary>
    /// Посадка
    /// </summary>
    void landing()
    {
        _client.enableApiControl(true);
        _client.landAsync()->waitOnLastTask();
    }

    /// <summary>
    /// Возвращает данные барометра
    /// </summary>
    BarometerBase::Output barometerData()
    {
        return _client.getBarometerData();
    }

    /// <summary>
    /// Возвращает данные ИНС
    /// </summary>
    ImuBase::Output imuData()
    {
        return _client.getImuData();
    }

    /// <summary>
    /// Возвращает данные GPS
    /// </summary>
    GpsBase::Output gpsData()
    {
        return _client.getGpsData();
    }

    /// <summary>
    /// Возвращает данные магнитометра
    /// </summary>
    MagnetometerBase::Output magnetometerData()
    {
        return _client.getMagnetometerData();
    }

    /// <summary>
    /// Возвращает изображение с камеры
    /// </summary>
    const std::vector<ImageResponse> cameraImage(const std::string& camera_name_val)
    {
        const std::vector<ImageRequest> request{ ImageRequest(camera_name_val, ImageType::Scene, false, true) };
        const std::vector<ImageResponse> response = _client.simGetImages(request);

        return response;
    }

    /// <summary>
    /// Возвращает значение глубины
    /// </summary>
    const std::vector<ImageResponse> cameraPixelsDepth(const std::string& camera_name_val)
    {
        const std::vector<ImageRequest> request{ ImageRequest(camera_name_val, ImageType::DepthPlanar, true, false) };
        const std::vector<ImageResponse> response = _client.simGetImages(request);

        return response;
    }

    /// <summary>
    /// Тестовый проверочный полёт
    /// </summary>
    void testFlyBox(const float speed = 3.0f, const float size = 10.0f)
    {
        _client.enableApiControl(true);

        auto position = _client.getMultirotorState().getPosition();
        float z = position.z(); // current position (NED coordinate system).
        const float duration = size / speed;
        DrivetrainType drivetrain = DrivetrainType::ForwardOnly;
        YawMode yaw_mode(true, 0);

        _client.moveByVelocityZAsync(speed, 0, z, duration, drivetrain, yaw_mode);
        std::this_thread::sleep_for(std::chrono::duration<double>(duration));
        _client.moveByVelocityZAsync(0, speed, z, duration, drivetrain, yaw_mode);
        std::this_thread::sleep_for(std::chrono::duration<double>(duration));
        _client.moveByVelocityZAsync(-speed, 0, z, duration, drivetrain, yaw_mode);
        std::this_thread::sleep_for(std::chrono::duration<double>(duration));
        _client.moveByVelocityZAsync(0, -speed, z, duration, drivetrain, yaw_mode);
        std::this_thread::sleep_for(std::chrono::duration<double>(duration));

        _client.hoverAsync()->waitOnLastTask();
    }

    /// <summary>
    /// Полёт вверх
    /// </summary>
    void toUpFly()
    {
        _client.enableApiControl(true);

        auto position = _client.getMultirotorState().getPosition();
        float z = position.z(); // current position (NED coordinate system).        
        const float size = 1.0f; // расстояние подъёма
        const float duration = size / _speed; // продолжительность манёвра
        // Установить угол рысканья
        YawMode yaw_mode;
        yaw_mode.setZeroRate(); 

        _client.moveByVelocityZAsync(0, 0, z - size, duration, _drivetrain, yaw_mode);
        std::this_thread::sleep_for(std::chrono::duration<double>(duration));


        _client.hoverAsync()->waitOnLastTask();
    }

    /// <summary>
    /// Полёт вниз
    /// </summary>
    void toDownFly()
    {
        _client.enableApiControl(true);

        auto position = _client.getMultirotorState().getPosition();
        float z = position.z(); // current position (NED coordinate system).
        const float size = 1.0f; // расстояние опускания
        const float duration = size / _speed; // продолжительность манёвра
        // Установить угол рысканья
        YawMode yaw_mode;
        yaw_mode.setZeroRate();

        _client.moveByVelocityZAsync(0, 0, z + size, duration, _drivetrain, yaw_mode);
        std::this_thread::sleep_for(std::chrono::duration<double>(duration));

        _client.hoverAsync()->waitOnLastTask();
    }

    /// <summary>
    /// Полёт вперёд
    /// </summary>
    void toForwardFly()
    {
        _client.enableApiControl(true);

        auto position = _client.getMultirotorState().getPosition();
        float z = position.z(); // current position (NED coordinate system).
        const float size = 1.0f; // расстояние 
        const float duration = size / _speed; // продолжительность манёвра
        // Установить угол рысканья
        YawMode yaw_mode;
        if (_drivetrain == DrivetrainType::ForwardOnly) {
            yaw_mode.is_rate = false;
            yaw_mode.yaw_or_rate = 0;
        }
        else {
            yaw_mode.setZeroRate();
        }

        _client.moveByVelocityZAsync(_speed, 0, z, duration, _drivetrain, yaw_mode);
        std::this_thread::sleep_for(std::chrono::duration<double>(duration));

        _client.hoverAsync()->waitOnLastTask();
    }

    /// <summary>
    /// Полёт вправо
    /// </summary>
    void toRightFly()
    {
        _client.enableApiControl(true);

        auto position = _client.getMultirotorState().getPosition();
        float z = position.z(); // current position (NED coordinate system).
        const float size = 1.0f; // расстояние
        const float duration = size / _speed; // продолжительность манёвра
        // Установить угол рысканья
        YawMode yaw_mode;
        if (_drivetrain == DrivetrainType::ForwardOnly) {
            yaw_mode.is_rate = false;
            yaw_mode.yaw_or_rate = 0;
        }
        else {
            yaw_mode.setZeroRate();
        }

        _client.moveByVelocityZAsync(0, _speed, z, duration, _drivetrain, yaw_mode);
        std::this_thread::sleep_for(std::chrono::duration<double>(duration));

        _client.hoverAsync()->waitOnLastTask();
    }

    /// <summary>
    /// Полёт влево
    /// </summary>
    void toLeftFly()
    {
        _client.enableApiControl(true);

        auto position = _client.getMultirotorState().getPosition();
        float z = position.z(); // current position (NED coordinate system).
        const float size = 1.0f; // расстояние
        const float duration = size / _speed; // продолжительность манёвра
        // Установить угол рысканья
        YawMode yaw_mode;
        if (_drivetrain == DrivetrainType::ForwardOnly) {
            yaw_mode.is_rate = false;
            yaw_mode.yaw_or_rate = 0;
        }
        else {
            yaw_mode.setZeroRate();
        }

        _client.moveByVelocityZAsync(0, -_speed, z, duration, _drivetrain, yaw_mode);
        std::this_thread::sleep_for(std::chrono::duration<double>(duration));

        _client.hoverAsync()->waitOnLastTask();
    }

    /// <summary>
    /// Полёт назад
    /// </summary>
    void toBackFly()
    {
        _client.enableApiControl(true);

        auto position = _client.getMultirotorState().getPosition();
        float z = position.z(); // current position (NED coordinate system).
        const float size = 1.0f; // расстояние
        const float duration = size / _speed; // продолжительность манёвра
        // Установить угол рысканья
        YawMode yaw_mode;
        if (_drivetrain == DrivetrainType::ForwardOnly) {
            yaw_mode.is_rate = false;
            yaw_mode.yaw_or_rate = 0;
        }
        else {
            yaw_mode.setZeroRate();
        }

        _client.moveByVelocityZAsync(-_speed, 0, z, duration, _drivetrain, yaw_mode);
        std::this_thread::sleep_for(std::chrono::duration<double>(duration));

        _client.hoverAsync()->waitOnLastTask();
    }

    /// <summary>
    /// Поворот
    /// </summary>
    void rotateByYaw(bool left = true)
    {
        _client.enableApiControl(true);
        const float duration = 1.0f;
        float yaw_rate = left ? 4.0f : -4.0f;
        if (_yaw_is_rate) {
            yaw_rate = left ? _yaw_or_rate : -_yaw_or_rate;
        }

        _client.rotateByYawRateAsync(yaw_rate, duration);
        std::this_thread::sleep_for(std::chrono::duration<double>(duration));

        _client.hoverAsync()->waitOnLastTask();
    }

    /// <summary>
    /// Расчёт расстояния от дрона до объекта на кадре
    /// </summary>
    double calculateDistanceToObject(const std::string& camera_name_val, 
                                     const std::pair<float, float> &object_center_pixel)
    {
        struct CameraIntrinsicMatrix
        {
            float fx; // фокусное расстояние по оси x
            float fy; // фокусное расстояние по оси y
            float cx; // смещение по оси x
            float cy; // смещение по оси y
        };

        // Получаем состояние дрона
        MultirotorState rotor_state = _client.getMultirotorState();
        Vector3r drone_position = rotor_state.getPosition();

        // Значение глубины (заменяется глубиной из сенсора)
        float pixel_depth = 10.0; // виртуальное значение глубины из карты
        const std::vector<ImageResponse> img_response = cameraPixelsDepth(camera_name_val);
        for (const ImageResponse& image_info : img_response) {
            const float *depth = reinterpret_cast<const float*>(image_info.image_data_float.data());
            pixel_depth = *(depth + 640 * 360);
        }

        // Внутренняя матрица камеры
        CameraIntrinsicMatrix camera_matrix = {
           1280, // Фокусное расстояние по горизонтали
            720, // Фокусное расстояние по вертикали
            640, // Центр по оси x
            360  // Центр по оси y
        };

        //
        // Рассчитываем расстояние
        float centerX = object_center_pixel.first;
        float centerY = object_center_pixel.second;

        // Мировые координаты объекта
        double Xw = (centerX - camera_matrix.cx) / camera_matrix.fx * pixel_depth + drone_position.x();
        double Yw = (centerY - camera_matrix.cy) / camera_matrix.fy * pixel_depth + drone_position.y();
        double Zw = pixel_depth + drone_position.z(); // считаем, что камера смотрит вперед

        // Расстояние от дрона до объекта
        double distance = std::sqrt(std::pow(Xw - drone_position.x(), 2) +
                                    std::pow(Yw - drone_position.y(), 2) +
                                    std::pow(Zw - drone_position.z(), 2));

        return distance;
    }

    };

}

#endif