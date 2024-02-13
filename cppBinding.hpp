#pragma once
#include <uWebSockets/App.h>

#define SSL 0
struct PerSocketData {};

void on_message(uWS::WebSocket<SSL, true, PerSocketData> *ws,
                std::string_view message, uWS::OpCode opCode);
void on_open(uWS::WebSocket<SSL, true, PerSocketData> *ws);
