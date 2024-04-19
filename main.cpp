#include "cppBinding.hpp"
#include "wispServer.hpp"
#include "wispValidation.hpp"
#include "wispnet.hpp"
#include <bits/getopt_core.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <iostream>
#include <string>

int port;

void showHelp(int ret) {

  printf("Usage: wispserver [options] <port>\n");
  printf(" --packet-ratelimit <limit>\t\t The ammount of socket packets that "
         "can be sent from a single websocket per minute.\n");
  printf(" --match-compatability <version>\t Disables features that need a "
         "higher version.\n");
  printf(" --help\t\t\t\t\t Shows this help menu.\n");
#ifdef DEBUG
  printf("\nCompiled with the debug flag\n");
#endif // DEBUG
#ifdef HASH
  printf("On commit hash " HASH "\n");
#endif // HASH
  exit(ret);
}

bool isValidInt(char *buf) {
  size_t buflen = strlen(buf);

  for (size_t i = 0; i < buflen; i++) {
    if (buf[i] > '9' || buf[i] < '0') {
      return false;
    }
  }
  return true;
}

int main(int argv, char *argc[]) {
  int option_index = 0;
  const struct option options[] = {
      {"packet-ratelimit", required_argument, NULL, 'c'},
      {"match-compatability", required_argument, NULL, 'm'},
      {"help", no_argument, NULL, 'h'},
      {NULL, 0, NULL, 0}};
  option_index = 0;
  int control;
  while ((control = getopt_long(argv, argc, "", options, &option_index)) !=
         -1) {
    switch (control) {
    case 'm': {
      if (!isValidInt(optarg)) {
        printf(
            "%s: for option --match-compatability value must be an integer.\n",
            argc[0]);
        showHelp(-1);
      }
      matchCompatability = std::stoi(optarg);
      break;
    }
    case 'c': {
      if (!isValidInt(optarg)) {
        printf("%s: for option --connect-ratelimit value must be an integer.\n",
               argc[0]);
        showHelp(-1);
      }
      maxForward = std::stoi(optarg);
      break;
    }
    case 'h':
    case '?':
    case ':':
    default:
      showHelp(-1);
    }
  }
  if (argv - optind != 1) {
    printf("%s: one regular argument is needed for port \n", argc[0]);
    showHelp(-1);
  }

  for (int i = optind; i < argv; i++) {
    if (!isValidInt(argc[i])) {
      printf("%s: port must be an integer\n", argc[0]);
      showHelp(-1);
    }
    port = std::stoi(argc[i]);
  }

  std::cout << "Starting on port " << port << " attempting Wisp V"
            << matchCompatability << "\n";

  init_wispnet();
  std::vector<std::thread *> threads(std::thread::hardware_concurrency());

  std::transform(
      threads.begin(), threads.end(), threads.begin(), [](std::thread * /*t*/) {
        return new std::thread([]() {
          uWS::TemplatedApp<false> app =
              uWS::App()
                  .ws<PerSocketData>(
                      "*",
                      {
                          .maxPayloadLength = 100 * 1024,
                          .upgrade =
                              [](auto *res, auto *req, auto *context) {
                                res->template upgrade<PerSocketData>(
                                    {}, req->getHeader("sec-websocket-key"), "",
                                    req->getHeader("sec-websocket-extensions"),
                                    context);
                              },
                          .open = [&app](
                                      uWS::WebSocket<SSL, true, PerSocketData>
                                          *ws) { on_open(&app, ws); },
                          .message = on_message,
                      })
                  .listen(port, [](auto *listen_socket) {
                    if (listen_socket) {
                    }
                  });
          app.run();
        });
      });

  std::for_each(threads.begin(), threads.end(),
                [](std::thread *t) { t->join(); });

  printf("Binding closed\n");
}
