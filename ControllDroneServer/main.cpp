// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

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
#include <locale>
#include <windows.h>

#include <compat/nanomsg/nn.h>
#include <compat/nanomsg/reqrep.h>

#include "DroneApplication.hpp"

int main()
{
    setlocale(LC_ALL, ".UTF-8");
    SetConsoleOutputCP(CP_UTF8);

    std::locale::global(std::locale(""));
    std::wcout.imbue(std::locale(""));

    drone::DroneApplication app;
    const std::string endpoint = "tcp://127.0.0.1:20001";
    if (app.initRpcControllServer(endpoint) < 0) {
        return -1;
    }

    return app.run();
}
