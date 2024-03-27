#pragma once
#include <uWebSockets/App.h>
#include <uWebSockets/Loop.h>

#define SSL 0
struct PerSocketData {
  uWS::Loop *loop;
};

void on_message(uWS::WebSocket<SSL, true, PerSocketData> *ws,
                std::string_view message, uWS::OpCode opCode);
void on_open(uWS::TemplatedApp<false> *app,
             uWS::WebSocket<SSL, true, PerSocketData> *ws);
