#include "server/webserver.h"

int main() {
  WebServer server("/media/psf/Home/Documents/project/server", 8787, 3, 60000,
                   false, 8, true, 1, 1024);
  server.Start();
}