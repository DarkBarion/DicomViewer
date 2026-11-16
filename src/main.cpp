#include <cstdio>
#include <exception>
#include <utility>

#include "Application.h"

int main(int argc, char* argv[]) {
  try {
    app::Application application(argc, argv);
    return application.Run();
  }
  catch (const std::bad_alloc&) {
    std::fprintf(stderr, "FATAL: out of memory\n");
    return std::to_underlying(app::ExitCode::kUnexpectedError);
  }
  catch (const std::exception& exception) {
    std::fprintf(stderr, "FATAL: %s\n", exception.what());
    return std::to_underlying(app::ExitCode::kUnexpectedError);
  }
  catch (...) {
    std::fprintf(stderr, "FATAL: unknown error\n");
    return std::to_underlying(app::ExitCode::kUnexpectedError);
  }
}
