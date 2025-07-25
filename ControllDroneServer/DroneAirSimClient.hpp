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
/// AirSim ������
/// </summary>
class DroneAirSimClient
{
private:
    MultirotorRpcLibClient _client;

public:
    bool _yaw_is_rate = false;
    float _yaw_or_rate = 0.0f; // ���� ��������
    float _speed = 5.0f; // ��������    
    DrivetrainType _drivetrain = DrivetrainType::ForwardOnly; // �������� �����

public:
    /// <summary>
    /// ���������� � �����������
    /// </summary>
    void connection()
    {
        _client.confirmConnection();
        _client.enableApiControl(true);
    }

    /// <summary>
    /// ���������/���������� �����
    /// </summary>
    void armDisarm(bool arm = true)
    {
        _client.enableApiControl(true);
        _client.armDisarm(arm);
    }

    /// <summary>
    /// ����
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
    /// �������
    /// </summary>
    void landing()
    {
        _client.enableApiControl(true);
        _client.landAsync()->waitOnLastTask();
    }

    /// <summary>
    /// ���������� ������ ���������
    /// </summary>
    BarometerBase::Output barometerData()
    {
        return _client.getBarometerData();
    }

    /// <summary>
    /// ���������� ������ ���
    /// </summary>
    ImuBase::Output imuData()
    {
        return _client.getImuData();
    }

    /// <summary>
    /// ���������� ������ GPS
    /// </summary>
    GpsBase::Output gpsData()
    {
        return _client.getGpsData();
    }

    /// <summary>
    /// ���������� ������ ������������
    /// </summary>
    MagnetometerBase::Output magnetometerData()
    {
        return _client.getMagnetometerData();
    }

    /// <summary>
    /// ���������� ����������� � ������
    /// </summary>
    const std::vector<ImageResponse> cameraImage(const std::string& camera_name_val)
    {
        const std::vector<ImageRequest> request{ ImageRequest(camera_name_val, ImageType::Scene, false, true) };
        const std::vector<ImageResponse> response = _client.simGetImages(request);

        return response;
    }

    /// <summary>
    /// ���������� �������� �������
    /// </summary>
    const std::vector<ImageResponse> cameraPixelsDepth(const std::string& camera_name_val)
    {
        const std::vector<ImageRequest> request{ ImageRequest(camera_name_val, ImageType::DepthPlanar, true, false) };
        const std::vector<ImageResponse> response = _client.simGetImages(request);

        return response;
    }

    /// <summary>
    /// �������� ����������� ����
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
    /// ���� �����
    /// </summary>
    void toUpFly()
    {
        _client.enableApiControl(true);

        auto position = _client.getMultirotorState().getPosition();
        float z = position.z(); // current position (NED coordinate system).        
        const float size = 1.0f; // ���������� �������
        const float duration = size / _speed; // ����������������� ������
        // ���������� ���� ��������
        YawMode yaw_mode;
        yaw_mode.setZeroRate(); 

        _client.moveByVelocityZAsync(0, 0, z - size, duration, _drivetrain, yaw_mode);
        std::this_thread::sleep_for(std::chrono::duration<double>(duration));


        _client.hoverAsync()->waitOnLastTask();
    }

    /// <summary>
    /// ���� ����
    /// </summary>
    void toDownFly()
    {
        _client.enableApiControl(true);

        auto position = _client.getMultirotorState().getPosition();
        float z = position.z(); // current position (NED coordinate system).
        const float size = 1.0f; // ���������� ���������
        const float duration = size / _speed; // ����������������� ������
        // ���������� ���� ��������
        YawMode yaw_mode;
        yaw_mode.setZeroRate();

        _client.moveByVelocityZAsync(0, 0, z + size, duration, _drivetrain, yaw_mode);
        std::this_thread::sleep_for(std::chrono::duration<double>(duration));

        _client.hoverAsync()->waitOnLastTask();
    }

    /// <summary>
    /// ���� �����
    /// </summary>
    void toForwardFly()
    {
        _client.enableApiControl(true);

        auto position = _client.getMultirotorState().getPosition();
        float z = position.z(); // current position (NED coordinate system).
        const float size = 1.0f; // ���������� 
        const float duration = size / _speed; // ����������������� ������
        // ���������� ���� ��������
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
    /// ���� ������
    /// </summary>
    void toRightFly()
    {
        _client.enableApiControl(true);

        auto position = _client.getMultirotorState().getPosition();
        float z = position.z(); // current position (NED coordinate system).
        const float size = 1.0f; // ����������
        const float duration = size / _speed; // ����������������� ������
        // ���������� ���� ��������
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
    /// ���� �����
    /// </summary>
    void toLeftFly()
    {
        _client.enableApiControl(true);

        auto position = _client.getMultirotorState().getPosition();
        float z = position.z(); // current position (NED coordinate system).
        const float size = 1.0f; // ����������
        const float duration = size / _speed; // ����������������� ������
        // ���������� ���� ��������
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
    /// ���� �����
    /// </summary>
    void toBackFly()
    {
        _client.enableApiControl(true);

        auto position = _client.getMultirotorState().getPosition();
        float z = position.z(); // current position (NED coordinate system).
        const float size = 1.0f; // ����������
        const float duration = size / _speed; // ����������������� ������
        // ���������� ���� ��������
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
    /// �������
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
    /// ������ ���������� �� ����� �� ������� �� �����
    /// </summary>
    double calculateDistanceToObject(const std::string& camera_name_val, 
                                     const std::pair<float, float> &object_center_pixel)
    {
        struct CameraIntrinsicMatrix
        {
            float fx; // �������� ���������� �� ��� x
            float fy; // �������� ���������� �� ��� y
            float cx; // �������� �� ��� x
            float cy; // �������� �� ��� y
        };

        // �������� ��������� �����
        MultirotorState rotor_state = _client.getMultirotorState();
        Vector3r drone_position = rotor_state.getPosition();

        // �������� ������� (���������� �������� �� �������)
        float pixel_depth = 10.0; // ����������� �������� ������� �� �����
        const std::vector<ImageResponse> img_response = cameraPixelsDepth(camera_name_val);
        for (const ImageResponse& image_info : img_response) {
            const float *depth = reinterpret_cast<const float*>(image_info.image_data_float.data());
            pixel_depth = *(depth + 640 * 360);
        }

        // ���������� ������� ������
        CameraIntrinsicMatrix camera_matrix = {
           1280, // �������� ���������� �� �����������
            720, // �������� ���������� �� ���������
            640, // ����� �� ��� x
            360  // ����� �� ��� y
        };

        //
        // ������������ ����������
        float centerX = object_center_pixel.first;
        float centerY = object_center_pixel.second;

        // ������� ���������� �������
        double Xw = (centerX - camera_matrix.cx) / camera_matrix.fx * pixel_depth + drone_position.x();
        double Yw = (centerY - camera_matrix.cy) / camera_matrix.fy * pixel_depth + drone_position.y();
        double Zw = pixel_depth + drone_position.z(); // �������, ��� ������ ������� ������

        // ���������� �� ����� �� �������
        double distance = std::sqrt(std::pow(Xw - drone_position.x(), 2) +
                                    std::pow(Yw - drone_position.y(), 2) +
                                    std::pow(Zw - drone_position.z(), 2));

        return distance;
    }

    };

}

#endif