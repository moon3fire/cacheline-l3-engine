#include "Application.h"

int main(int argc, char* argv[]) {
    cacheline::Application app;

    if (!app.Initialize(argc, argv)) {
        return EXIT_FAILURE;
    }

    if (app.ShouldRunSelfTest()) {
        return app.RunSelfTest() ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    app.Run();

    return EXIT_SUCCESS;
}