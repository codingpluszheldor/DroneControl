#ifndef DRONE_APPLICATION_HPP
#define DRONE_APPLICATION_HPP

#include "common/common_utils/StrictMode.hpp"
STRICT_MODE_OFF
#ifndef RPCLIB_MSGPACK
#define RPCLIB_MSGPACK clmdep_msgpack
#endif // !RPCLIB_MSGPACK
#include "rpc/rpc_error.h"

// Отключаем специфичные предупреждения перед включением OpenCV
//#pragma warning(push)
//#pragma warning(disable : 4127) // C4127: conditional expression is constant
//#pragma warning(disable : 4996) // C4996: deprecated functions
//#pragma warning(disable : 4266) // C4266: no override available for virtual member function
//#pragma warning(disable : 4263) // C4263: member function does not override any base class virtual member function
//#pragma warning(disable : 4267) // C4267: conversion from size_t to int, possible loss of data
//#pragma warning(disable : 4100) // C4100: unreferenced formal parameter
//#include <opencv2/core.hpp>
//#include <opencv2/imgcodecs.hpp>
//#include <opencv2/imgproc.hpp>
//#include <opencv2/calib3d.hpp>
//#pragma warning(pop)

STRICT_MODE_ON

#include "vehicles/multirotor/api/MultirotorRpcLibClient.hpp"
#include "common/common_utils/FileSystem.hpp"
#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include <atomic>

#include <compat/nanomsg/nn.h>
#include <compat/nanomsg/reqrep.h>
#include <compat/nanomsg/pipeline.h>

#include "DroneAirSimClient.hpp"
#include "DroneRpc.hpp"
#include "SafeMessageQueue.hpp"

using namespace msr::airlib;

namespace drone
{

void receiveMessages(int sock_fd, SafeMessageQueue<std::vector<std::byte>> &incoming_queue)
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

void sendResponses(int sock_fd, SafeMessageQueue<std::vector<std::byte>> &outgoing_queue)
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
    int _client_sock = -1;
    std::atomic<bool> _get_image{ false };
    std::atomic<bool> _running{ true };
    std::string _camera_name_val = "front-center";
    ImageCaptureBase::ImageType _camera_img_type = ImageType::Scene;

    // Параметры стерео камеры
    //cv::Mat _camera_matrix_left;
    //cv::Mat _camera_matrix_right;
    //cv::Mat _dist_coeffs_left;
    //cv::Mat _dist_coeffs_right;
    //cv::Mat _R;
    //cv::Mat _T;

    //// Карты для ректификации
    //cv::Mat _left_map1, _left_map2, _right_map1, _right_map2;
    //bool _stereo_initialized = false;

public:
    DroneApplication()
    {
        // Инициализация параметров стерео камеры
        //initializeStereoParameters();
    }

    ///// <summary>
    ///// Инициализация параметров стерео камеры
    ///// </summary>
    //void initializeStereoParameters()
    //{
    //    // Примерные параметры камеры
    //    _camera_matrix_left = (cv::Mat_<double>(3, 3) << 1000.0, 0.0, 320.0, 0.0, 1000.0, 240.0, 0.0, 0.0, 1.0);

    //    _camera_matrix_right = (cv::Mat_<double>(3, 3) << 1000.0, 0.0, 320.0, 0.0, 1000.0, 240.0, 0.0, 0.0, 1.0);

    //    _dist_coeffs_left = cv::Mat::zeros(1, 5, CV_64F);
    //    _dist_coeffs_right = cv::Mat::zeros(1, 5, CV_64F);

    //    _R = cv::Mat::eye(3, 3, CV_64F);
    //    _T = (cv::Mat_<double>(3, 1) << -0.1, 0.0, 0.0);
    //}

    ///// <summary>
    ///// Инициализация стерео ректификации
    ///// </summary>
    //bool initializeStereoRectification(const cv::Size& image_size)
    //{
    //    try {
    //        cv::Mat R1, R2, P1, P2, Q;
    //        cv::stereoRectify(_camera_matrix_left, _dist_coeffs_left, _camera_matrix_right, _dist_coeffs_right, image_size, _R, _T, R1, R2, P1, P2, Q, cv::CALIB_ZERO_DISPARITY, 0, image_size);

    //        cv::initUndistortRectifyMap(_camera_matrix_left, _dist_coeffs_left, R1, P1, image_size, CV_16SC2, _left_map1, _left_map2);
    //        cv::initUndistortRectifyMap(_camera_matrix_right, _dist_coeffs_right, R2, P2, image_size, CV_16SC2, _right_map1, _right_map2);

    //        _stereo_initialized = true;
    //        return true;
    //    }
    //    catch (const cv::Exception& e) {
    //        std::cerr << "Ошибка инициализации стерео ректификации: " << e.what() << "\n";
    //        return false;
    //    }
    //}

    ///// <summary>
    ///// Безопасное преобразование size_t в int для OpenCV
    ///// </summary>
    //int safeSizeToInt(size_t size_val)
    //{
    //    return static_cast<int>(size_val);
    //}

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
    void airSimApi(const DroneMethods& method)
    {
        try {
            switch (method) {
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
            case DroneMethods::RotateLeft: {
                _client.rotateByYaw();
                makeResponseControl(DroneMethods::RotateLeft);
                break;
            }
            case DroneMethods::RotateRight: {
                _client.rotateByYaw(false);
                makeResponseControl(DroneMethods::RotateRight);
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
    /// Отправка изображения с камеры
    /// </summary>
    void cameraImageLoop()
    {
        _client_sock = nn_socket(AF_SP, NN_PUSH);
        if (_client_sock < 0) {
            std::cerr << "Ошибка инициализации сокета для отправки данных с камеры\n";
            return;
        }
        const char* endpoint = "tcp://127.0.0.1:20002";
        if (nn_connect(_client_sock, endpoint) < 0) {
            std::cerr << "Ошибка соединения к серверудля отправки данных с камеры\n";
            nn_close(_client_sock);
            return;
        }

        std::cout << "Камеры готова... " << '\n';
        using namespace std::chrono_literals;
        // Расчёт задержки
        //const float delay = (1.0f / 15.0f) * 1000.0f; // 15 fps (кадров/сек) 
        while (_running) {
            try {
                if (_get_image) {
                    //std::cout << "Запрос изображения от камеры... " << '\n';
                    if (_camera_name_val != "stereo") {
                        const std::vector<ImageResponse> img_response = _client.cameraImage(_camera_name_val, _camera_img_type);
                        for (const ImageResponse& image_info : img_response) {
                            int send_result = nn_send(_client_sock,
                                                      reinterpret_cast<const char*>(image_info.image_data_uint8.data()),
                                                      image_info.image_data_uint8.size(),
                                                      0);
                            //std::cout << "Отравлено от камеры, размер: " << image_info.image_data_uint8.size() << '\n';
                            if (send_result < 0) {
                                std::cerr << "Ошибка отправки данных с камеры в сокет\n";
                            }
                        }
                        //std::this_thread::sleep_for(std::chrono::duration<double>(delay));
                        // std::this_thread::sleep_for(15ms);
                    }
                    else {
                        //const std::vector<ImageResponse> img_response_left = _client.cameraImage(map_cameras[DroneCamera::front_left], _camera_img_type);
                        //const std::vector<ImageResponse> img_response_right = _client.cameraImage(map_cameras[DroneCamera::front_right], _camera_img_type);

                        //// вычисления Depth Planar
                        //std::vector<std::byte> depth_data = computeDepthPlanar(img_response_left, img_response_right);
                        //if (!depth_data.empty()) {
                        //    // Отправляем depth изображение
                        //    int send_result = nn_send(_client_sock,
                        //                              reinterpret_cast<const char*>(depth_data.data()),
                        //                              depth_data.size(),
                        //                              0);
                        //    if (send_result < 0) {
                        //        std::cerr << "Ошибка отправки depth изображения\n";
                        //    }
                        //    
                        //}

                        const std::vector<ImageResponse> img_response_left = _client.cameraImage(map_cameras[DroneCamera::front_left], _camera_img_type);
                        for (const ImageResponse& image_info : img_response_left) {
                            std::string path = "D:\\Documents\\AirSim\\StereoRecordings\\Left";
                            std::string file_path = FileSystem::combine(path, std::to_string(image_info.time_stamp));
                            std::ofstream file(file_path + ".png", std::ios::binary);
                            file.write(reinterpret_cast<const char*>(image_info.image_data_uint8.data()), image_info.image_data_uint8.size());
                            file.close();
                        }

                        const std::vector<ImageResponse> img_response_right = _client.cameraImage(map_cameras[DroneCamera::front_right], _camera_img_type);
                        for (const ImageResponse& image_info : img_response_right) {
                            std::string path = "D:\\Documents\\AirSim\\StereoRecordings\\Right";
                            std::string file_path = FileSystem::combine(path, std::to_string(image_info.time_stamp));
                            std::ofstream file(file_path + ".png", std::ios::binary);
                            file.write(reinterpret_cast<const char*>(image_info.image_data_uint8.data()), image_info.image_data_uint8.size());
                            file.close();
                        }
                    }

                    // VAS: test to files
                    //for (const ImageResponse& image_info : img_response) {
                    //    std::string path = "D:\\Documents\\AirSim\\Recordings";
                    //    std::string file_path = FileSystem::combine(path, std::to_string(image_info.time_stamp));
                    //    std::ofstream file(file_path + ".png", std::ios::binary);
                    //    file.write(reinterpret_cast<const char*>(image_info.image_data_uint8.data()), image_info.image_data_uint8.size());
                    //    file.close();
                    //}
                }
                else {
                    std::this_thread::sleep_for(1s);
                    //std::cout << "Ожидание включения камеры камеры... " << '\n';
                }
            }
            catch (...) {
                std::cerr << "Ошибка получения и отправки данных с камеры\n";
            }
        }
    }

    ///// <summary>
    ///// Функция для вычисления Depth Planar из стерео изображений
    ///// </summary>
    ///// <param name="img_response_left">Левый кадр стерео пары</param>
    ///// <param name="img_response_right">Правый кадр стерео пары</param>
    ///// <returns>Вектор байтов с данными depth изображения</returns>
    //std::vector<std::byte> computeDepthPlanar(const std::vector<ImageResponse>& img_response_left,
    //                                          const std::vector<ImageResponse>& img_response_right)
    //{
    //    if (img_response_left.empty() || img_response_right.empty()) {
    //        std::cerr << "Пустые данные стерео изображений\n";
    //        return {};
    //    }

    //    try {
    //        // Декодируем изображения из буфера в cv::Mat
    //        const auto& left_data = img_response_left[0].image_data_uint8;
    //        const auto& right_data = img_response_right[0].image_data_uint8;

    //        // Безопасное преобразование size_t в int
    //        int left_size = safeSizeToInt(left_data.size());
    //        int right_size = safeSizeToInt(right_data.size());

    //        cv::Mat left_img = cv::imdecode(
    //            cv::Mat(1, left_size, CV_8UC1, const_cast<uint8_t*>(left_data.data())),
    //            cv::IMREAD_GRAYSCALE);

    //        cv::Mat right_img = cv::imdecode(
    //            cv::Mat(1, right_size, CV_8UC1, const_cast<uint8_t*>(right_data.data())),
    //            cv::IMREAD_GRAYSCALE);

    //        if (left_img.empty() || right_img.empty()) {
    //            std::cerr << "Не удалось декодировать стерео изображения\n";
    //            return {};
    //        }

    //        // Инициализируем ректификацию при первом вызове
    //        if (!_stereo_initialized) {
    //            if (!initializeStereoRectification(left_img.size())) {
    //                return {};
    //            }
    //        }

    //        // Ректификация стерео изображений
    //        cv::Mat left_rectified, right_rectified;
    //        cv::remap(left_img, left_rectified, _left_map1, _left_map2, cv::INTER_LINEAR);
    //        cv::remap(right_img, right_rectified, _right_map1, _right_map2, cv::INTER_LINEAR);

    //        // Вычисление карты диспаратности
    //        auto stereo = cv::StereoBM::create();
    //        stereo->setNumDisparities(80);
    //        stereo->setBlockSize(15);

    //        cv::Mat disparity;
    //        stereo->compute(left_rectified, right_rectified, disparity);

    //        // Конвертируем disparity в depth
    //        cv::Mat depth_planar;
    //        disparity.convertTo(disparity, CV_32F, 1.0 / 16.0);

    //        // Вычисляем глубину
    //        double baseline = cv::norm(_T);
    //        double focal_length = _camera_matrix_left.at<double>(0, 0);

    //        cv::Mat depth_map = (focal_length * baseline) / (disparity + 1e-6);

    //        // Нормализуем depth map
    //        double min_val, max_val;
    //        cv::minMaxLoc(depth_map, &min_val, &max_val);

    //        if (max_val > min_val) {
    //            depth_map.convertTo(depth_planar, CV_8UC1, 255.0 / (max_val - min_val), -min_val * 255.0 / (max_val - min_val));
    //        }
    //        else {
    //            depth_planar = cv::Mat::zeros(depth_map.size(), CV_8UC1);
    //        }

    //        // Кодируем depth изображение в PNG
    //        std::vector<uchar> encoded_depth;
    //        if (!cv::imencode(".png", depth_planar, encoded_depth)) {
    //            std::cerr << "Ошибка кодирования depth изображения\n";
    //            return {};
    //        }

    //        // Конвертируем в std::vector<std::byte>
    //        std::vector<std::byte> depth_data(encoded_depth.size());
    //        memcpy(depth_data.data(), encoded_depth.data(), encoded_depth.size());

    //        std::cout << "Depth Planar вычислен, размер: " << depth_data.size() << " байт\n";
    //        return depth_data;
    //    }
    //    catch (const cv::Exception& e) {
    //        std::cerr << "OpenCV ошибка при вычислении depth: " << e.what() << "\n";
    //        return {};
    //    }
    //    catch (const std::exception& e) {
    //        std::cerr << "Ошибка при вычислении depth: " << e.what() << "\n";
    //        return {};
    //    }
    //}

    /// <summary>
    /// Запуск цикла сообщений
    /// </summary>
    int run()
    {
        std::thread receiver_thread(receiveMessages, _server_sock, std::ref(_incoming_queue));
        std::thread sender_thread(sendResponses, _server_sock, std::ref(_outgoing_queue));
        std::thread cam_image_thread(&DroneApplication::cameraImageLoop, this);

        try {
            while (true) {
                // Ожидание сообщения от клиента
                std::vector<std::byte> incoming_message = _incoming_queue.pop();
                std::cout << "Получено сообщение, размер: " << incoming_message.size() << '\n';

                _response.clear();
                DroneMethodReq *request = reinterpret_cast<DroneMethodReq*>(incoming_message.data());
                if (request != nullptr) {
                    _get_image = request->get_camera_image;
                    if (_get_image) {
                        _camera_name_val = map_cameras[request->camera];
                        _camera_img_type = cameraImageType(request->camera_image_type);
                        std::cout << "Камера включена !!!" << '\n';
                    }
                    else {
                        std::cout << "Камера выключена" << '\n';
                    }
                    airSimParams(request);
                    airSimApi(request->method);
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

        _running = false;
        receiver_thread.join();
        sender_thread.join();
        cam_image_thread.join();

        nn_shutdown(_server_sock, 0);
        nn_close(_server_sock);
        nn_shutdown(_client_sock, 0);
        nn_close(_client_sock);

        return 0;
    }

private:
    /// <summary>
    /// Создание структуры ответа на упраляющую команду
    /// </summary>
    void makeResponseControl(const DroneMethods method)
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
        }
        
        _response = std::vector<std::byte>(reinterpret_cast<const std::byte*>(reply),
                                           reinterpret_cast<const std::byte*>(reply + sizeof(DroneReply)));
        delete reply;
    }

    /// <summary>
    /// Возвращает тип изображения камеры
    /// </summary>
    ImageCaptureBase::ImageType cameraImageType(const DroneImageType type)
    {
        switch (type) {
        case DroneImageType::Scene:
            return ImageType::Scene;
        case DroneImageType::DepthPlanar:
            return ImageType::DepthPlanar;
        case DroneImageType::DepthPerspective:
            return ImageType::DepthPerspective;
        case DroneImageType::Segmentation:
            return ImageType::Segmentation;
        default:
            return ImageType::Scene;
        }
    }
};

}

#endif