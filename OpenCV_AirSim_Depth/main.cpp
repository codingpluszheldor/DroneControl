#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <dirent.h>

using namespace cv;
using namespace std;

// Параметры стерео камеры
cv::Mat _camera_matrix_left;
cv::Mat _camera_matrix_right;
cv::Mat _dist_coeffs_left;
cv::Mat _dist_coeffs_right;
cv::Mat _R;
cv::Mat _T;

// Карты для ректификации
cv::Mat _left_map1, _left_map2, _right_map1, _right_map2;
bool _stereo_initialized = false;

/// <summary>
/// Инициализация параметров стерео камеры
/// </summary>
void initializeStereoParameters()
{
    // Примерные параметры камеры
    _camera_matrix_left = (cv::Mat_<double>(3, 3) << 1000.0, 0.0, 320.0, 0.0, 1000.0, 240.0, 0.0, 0.0, 1.0);

    _camera_matrix_right = (cv::Mat_<double>(3, 3) << 1000.0, 0.0, 320.0, 0.0, 1000.0, 240.0, 0.0, 0.0, 1.0);

    _dist_coeffs_left = cv::Mat::zeros(1, 5, CV_64F);
    _dist_coeffs_right = cv::Mat::zeros(1, 5, CV_64F);

    _R = cv::Mat::eye(3, 3, CV_64F);
    _T = (cv::Mat_<double>(3, 1) << -0.1, 0.0, 0.0);
}

/// <summary>
/// Инициализация стерео ректификации
/// </summary>
bool initializeStereoRectification(const cv::Size& image_size)
{
    try {
        cv::Mat R1, R2, P1, P2, Q;
        cv::stereoRectify(_camera_matrix_left, _dist_coeffs_left, _camera_matrix_right, _dist_coeffs_right, image_size, _R, _T, R1, R2, P1, P2, Q, cv::CALIB_ZERO_DISPARITY, 0, image_size);

        cv::initUndistortRectifyMap(_camera_matrix_left, _dist_coeffs_left, R1, P1, image_size, CV_16SC2, _left_map1, _left_map2);
        cv::initUndistortRectifyMap(_camera_matrix_right, _dist_coeffs_right, R2, P2, image_size, CV_16SC2, _right_map1, _right_map2);

        _stereo_initialized = true;
        return true;
    }
    catch (const cv::Exception& e) {
        std::cerr << "Ошибка инициализации стерео ректификации: " << e.what() << "\n";
        return false;
    }
}

/// <summary>
/// Получение списка файлов из папки, отсортированных по имени
/// </summary>
vector<string> getSortedFilesFromFolder(const string& folder_path)
{
    vector<string> files;

    DIR* dir;
    struct dirent* ent;

    if ((dir = opendir(folder_path.c_str())) != nullptr) {
        while ((ent = readdir(dir)) != nullptr) {
            string filename = ent->d_name;

            // Пропускаем текущую и родительскую директории
            if (filename == "." || filename == "..") {
                continue;
            }

            // Проверяем, что это файл (упрощенная проверка)
            string full_path = folder_path + filename;
            files.push_back(full_path);
        }
        closedir(dir);

        // Сортируем файлы по имени
        sort(files.begin(), files.end());

        cout << "Найдено файлов в папке " << folder_path << ": " << files.size() << endl;

        // Выводим первые несколько файлов для проверки
        if (!files.empty()) {
            cout << "Первые 3 файла:" << endl;
            for (size_t i = 0; i < min(files.size(), size_t(3)); ++i) {
                cout << "  " << files[i] << endl;
            }
        }
    }
    else {
        cerr << "Ошибка: не удалось открыть папку " << folder_path << endl;
    }

    return files;
}

/// <summary>
/// Функция для вычисления Depth Planar из стерео изображений
/// </summary>
cv::Mat computeDepthPlanar(const cv::Mat& left_img, const cv::Mat& right_img)
{
    if (left_img.empty() || right_img.empty()) {
        std::cerr << "Пустые данные стерео изображений\n";
        return cv::Mat();
    }

    try {
        // Инициализируем ректификацию при первом вызове
        if (!_stereo_initialized) {
            if (!initializeStereoRectification(left_img.size())) {
                return cv::Mat();
            }
        }

        // Ректификация стерео изображений
        cv::Mat left_rectified, right_rectified;
        cv::remap(left_img, left_rectified, _left_map1, _left_map2, cv::INTER_LINEAR);
        cv::remap(right_img, right_rectified, _right_map1, _right_map2, cv::INTER_LINEAR);

        // Вычисление карты диспаратности
        auto stereo = cv::StereoBM::create();
        stereo->setNumDisparities(80);
        stereo->setBlockSize(15);

        cv::Mat disparity;
        stereo->compute(left_rectified, right_rectified, disparity);

        // Конвертируем disparity в depth
        cv::Mat depth_planar;
        disparity.convertTo(disparity, CV_32F, 1.0 / 16.0);

        // Вычисляем глубину
        double baseline = cv::norm(_T);
        double focal_length = _camera_matrix_left.at<double>(0, 0);

        cv::Mat depth_map = (focal_length * baseline) / (disparity + 1e-6);

        // Нормализуем depth map
        double min_val, max_val;
        cv::minMaxLoc(depth_map, &min_val, &max_val);

        if (max_val > min_val) {
            depth_map.convertTo(depth_planar, CV_8UC1, 255.0 / (max_val - min_val), -min_val * 255.0 / (max_val - min_val));
        }
        else {
            depth_planar = cv::Mat::zeros(depth_map.size(), CV_8UC1);
        }

        return depth_planar;
    }
    catch (const cv::Exception& e) {
        std::cerr << "OpenCV ошибка при вычислении depth: " << e.what() << "\n";
        return cv::Mat();
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка при вычислении depth: " << e.what() << "\n";
        return cv::Mat();
    }
}

int main()
{
    // Инициализация параметров стерео камеры
    initializeStereoParameters();

    // Пути к папкам с изображениями
    string left_folder = "d:/Documents/AirSim/StereoRecordings/Left/";
    string right_folder = "d:/Documents/AirSim/StereoRecordings/Right/";

    // Получаем списки файлов
    vector<string> left_files = getSortedFilesFromFolder(left_folder);
    vector<string> right_files = getSortedFilesFromFolder(right_folder);

    if (left_files.empty() || right_files.empty()) {
        cerr << "Ошибка: не найдены файлы в одной из папок" << endl;
        return -1;
    }

    // Используем std::min из C++17
    const size_t total_pairs = min(left_files.size(), right_files.size());

    if (total_pairs == 0) {
        cerr << "Ошибка: нет пар для обработки" << endl;
        return -1;
    }

    // Читаем первое изображение для определения размера
    Mat first_left = imread(left_files[0], IMREAD_GRAYSCALE);
    if (first_left.empty()) {
        cerr << "Ошибка: не удалось прочитать первое изображение" << endl;
        return -1;
    }

    // Создаем VideoWriter для сохранения результата
    VideoWriter writer;
    string output_filename = "d:/Documents/AirSim/depth_output.avi";
    int codec = VideoWriter::fourcc('M', 'J', 'P', 'G');
    double fps = 30.0; // Предполагаемый FPS

    writer.open(output_filename, codec, fps, first_left.size(), false); // false для grayscale

    if (!writer.isOpened()) {
        // Пробуем другой кодек
        codec = VideoWriter::fourcc('X', 'V', 'I', 'D');
        writer.open(output_filename, codec, fps, first_left.size(), false);

        if (!writer.isOpened()) {
            cerr << "Ошибка: не удалось создать выходной файл" << endl;
            return -1;
        }
        cout << "Используется кодек XVID" << endl;
    } else {
        cout << "Используется кодек MJPG" << endl;
    }

    cout << "Обработка стереопар..." << endl;
    cout << "Всего пар для обработки: " << total_pairs << endl;
    cout << "Размер кадра: " << first_left.cols << "x" << first_left.rows << endl;
    cout << "FPS выходного видео: " << fps << endl;

    size_t processed_count = 0;
    size_t success_count = 0;

    for (size_t i = 0; i < total_pairs; i++) {
        // Читаем левое и правое изображения
        Mat left_img = imread(left_files[i], IMREAD_GRAYSCALE);
        Mat right_img = imread(right_files[i], IMREAD_GRAYSCALE);

        if (left_img.empty() || right_img.empty()) {
            cerr << "Ошибка: не удалось прочитать пару " << i << endl;
            cerr << "Левый файл: " << left_files[i] << endl;
            cerr << "Правый файл: " << right_files[i] << endl;
            processed_count++;
            continue;
        }

        // Проверяем размеры изображений
        if (left_img.size() != right_img.size()) {
            cerr << "Предупреждение: размеры изображений пары " << i << " не совпадают" << endl;
            cerr << "Левый: " << left_img.cols << "x" << left_img.rows << endl;
            cerr << "Правый: " << right_img.cols << "x" << right_img.rows << endl;
            processed_count++;
            continue;
        }

        // Вычисляем карту глубины
        Mat depth_planar = computeDepthPlanar(left_img, right_img);

        if (depth_planar.empty()) {
            cerr << "Ошибка: не удалось вычислить глубину для пары " << i << endl;
            processed_count++;
            continue;
        }

        // Сохраняем результат (grayscale)
        writer.write(depth_planar);

        success_count++;
        processed_count++;

        // Показываем прогресс
        if (processed_count % 10 == 0 || processed_count == total_pairs) {
            cout << "Обработано пар: " << processed_count << "/" << total_pairs
                 << " (успешно: " << success_count << ")" << endl;
        }

        // Показываем результат в реальном времени (опционально)
        Mat depth_colored;
        applyColorMap(depth_planar, depth_colored, COLORMAP_JET);

        imshow("Left Image", left_img);
        imshow("Right Image", right_img);
        imshow("Depth Planar", depth_colored);

        // Выход по нажатию 'q'
        int key = waitKey(1);
        if (key == 'q' || key == 27) { // 'q' или ESC
            cout << "Прервано пользователем" << endl;
            break;
        }
    }

    // Освобождаем ресурсы
    writer.release();
    destroyAllWindows();

    cout << "Обработка завершена!" << endl;
    cout << "Успешно обработано пар: " << success_count << "/" << total_pairs << endl;
    cout << "Результат сохранен в: " << output_filename << endl;

    return 0;
}
