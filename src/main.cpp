#include <math.h>
#include <uWS/uWS.h>
#include <iostream>
#include <ctime>

#include <string>
#include "json.hpp"
#include "PID.h"
#include "dual_stream.h"

// for convenience
using nlohmann::json;
using std::string;
using std::endl;
using std::cout;

// For converting back and forth between radians and degrees.
constexpr double pi() { return M_PI; }
double deg2rad(double x) { return x * pi() / 180; }
double rad2deg(double x) { return x * 180 / pi(); }

// Checks if the SocketIO event has JSON data.
// If there is data the JSON object in string format will be returned,
// else the empty string "" will be returned.
string hasData(string s) {
  auto found_null = s.find("null");
  auto b1 = s.find_first_of("[");
  auto b2 = s.find_last_of("]");
  if (found_null != string::npos) {
    return "";
  }
  else if (b1 != string::npos && b2 != string::npos) {
    return s.substr(b1, b2 - b1 + 1);
  }
  return "";
}

int main() {
  uWS::Hub h;

  PID pid;
  //Initialize the pid variable.
  pid.Init({0.369406 , 14.1166 , 0.00453171});
  // Use this initialization instead of the standard when you want to tune the PID constants with twiddle
  //pid.InitTuning({0.15, 7.0,0.003}, 100, 0.00002);


  dual_stream ds("datalog.txt");

  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);
  ds << std::put_time(&tm, "%d-%m-%Y %H-%M-%S\n") << endl;

  h.onMessage([&pid, &ds](uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length, 
                     uWS::OpCode opCode) {
    // "42" at the start of the message means there's a websocket message event.
    // The 4 signifies a websocket message
    // The 2 signifies a websocket event
    if (length && length > 2 && data[0] == '4' && data[1] == '2') {
      auto s = hasData(string(data).substr(0, length));

      if (s != "") {
        json j = json::parse(s);

        string event = j[0].get<string>();

        if (event == "telemetry") {
          
          string msg;

          if (pid.isTuningEnable()) {
            msg = pid.TwiddleTunning( j, ds); 
          } else {
            msg = pid.runProcess(j);
          }

          ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);

        }  // end "telemetry" if
      } else {
        // Manual driving
        string msg = "42[\"manual\",{}]";
        ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
      }
    }  // end websocket message if
  }); // end h.onMessage

  h.onConnection([&h](uWS::WebSocket<uWS::SERVER> ws, uWS::HttpRequest req) {
    std::cout << "Connected!!!" << std::endl;
  });

  h.onDisconnection([&h](uWS::WebSocket<uWS::SERVER> ws, int code, 
                         char *message, size_t length) {
    ws.close();
    std::cout << "Disconnected" << std::endl;
  });

  int port = 4567;
  if (h.listen(port)) {
    std::cout << "Listening to port " << port << std::endl;
  } else {
    std::cerr << "Failed to listen to port" << std::endl;
    return -1;
  }
  
  h.run();
}