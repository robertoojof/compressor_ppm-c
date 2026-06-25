#include "menus/CLIApplication.hpp"
#include "menus/DecodeCommand.hpp"
#include "menus/EncodeCommand.hpp"
#include <memory>

using namespace std;

int main(int argc, char *argv[]) {
  CLIApplication app(argv[0]);
  app.registerCommand("-encode", make_unique<EncodeCommand>());
  app.registerCommand("-decode", make_unique<DecodeCommand>());
  app.run(argc, argv);
  return 0;
}
