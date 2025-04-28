#include <iostream>
#include <string>
#include <windows.h>
#include <compat/nanomsg/nn.h>
#include <compat/nanomsg/reqrep.h>

int main() {
    setlocale(LC_ALL, ".UTF-8");
    SetConsoleOutputCP(CP_UTF8);

    int client_sock = nn_socket(AF_SP, NN_REQ);   // Запрашиваем данные у сервера
    if(client_sock < 0) {
        std::cerr << "Ошибка инициализации socket\n";
        return 1;
    }

    const char* endpoint = "tcp://127.0.0.1:20001";  // Подключаемся к серверу
    if(nn_connect(client_sock, endpoint) < 0) {
        std::cerr << "Ошибка соединения с сервером\n";
        nn_close(client_sock);
        return 1;
    }

    std::string message = "Привет, сервер!";
    int send_result = nn_send(client_sock, message.data(), message.size(), 0);
    if(send_result < 0) {
        std::cerr << "Ошибка отправки сообщения\n";
    } else {
        std::cout << "Сообщение успешно отправлено!\n";

        // Ожидание ответа от сервера
        char buffer[1024];
        int recv_result = nn_recv(client_sock, buffer, sizeof(buffer), 0);
        if(recv_result > 0){
            std::cout << "Полученный ответ от сервера: " << buffer << "\n";
        } else {
            std::cerr << "Ошибка приема ответа\n";
        }
    }

    nn_shutdown(client_sock, 0);
    nn_close(client_sock);
    return 0;
}
