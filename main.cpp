#include "server/webserver.h"

int main() {
  WebServer server(8787, 3, 60000, false, 8, false, 1, 1024);
  server.Start();
}