#include "libCE/Client.hpp"
#include <functional>
#include <string>
#include <thread>
#include <unistd.h>
#ifdef SDL_ENABLED
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#endif

int main(int argc, char* argv[]) {
    std::string name;
    if (argc == 2) {
        name = argv[1];
    }
    else {        
        char hostname[HOST_NAME_MAX];
        gethostname(hostname, HOST_NAME_MAX+1);
        name = hostname;
    }
    auto client = Client(name);
    #ifdef SDL_ENABLED

        if (!SDL_Init(SDL_INIT_VIDEO)) {
            std::cerr << "Failed to start SDL: " << SDL_GetError() << ". Some devices may not work properly." << std::endl;
        }

        SDL_Window* window = nullptr;
        SDL_Renderer* renderer = nullptr;
        auto windowName = name + " Input Window";
        if (!SDL_CreateWindowAndRenderer(windowName.c_str(), 800, 600, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY, &window, &renderer)) {
            std::cerr << "Input window creation failed: " << SDL_GetError() << ". Some devices may not work properly." << std::endl;
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);

        std::jthread clientEngine(std::bind(&Client::start, std::ref(client)));

        while (true) {
            SDL_Event event;
            SDL_WaitEvent(&event);
            switch (event.type) {
                case SDL_EVENT_QUIT: // for now closing the input window will stop the client
                    client.disconnect();
                    goto exit;
                break;
                case SDL_EVENT_USER: // if no SDL devices are created, stop the SDL subsystem
                    goto exit;
                break;
            }
        }
        exit: ;

        SDL_Quit();
        clientEngine.join();
        
        //client.start();
    #else
        client.start();
    #endif
}