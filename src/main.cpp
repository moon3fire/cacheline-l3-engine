#include "Application.h"

int main(int argc, char* argv[]) {
    cacheline::Application app;

    if (!app.initialize(argc, argv)) {
        return EXIT_FAILURE;
    }

    app.run();

    return EXIT_SUCCESS;
}