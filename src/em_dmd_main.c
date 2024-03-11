#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <assert.h>


#define PREFIX "Evolution Machine DMD Display"
#define DMD_WIDTH	(2716)
#define DMD_HEIGHT	(1600)
#define DMD_SIZE	(DMD_WIDTH * DMD_HEIGHT)
#define IMG_SIZE	(DMD_SIZE * 3)
#define INIT_VALUE	(255)
#define PORT 		(12345) 				// If something fails: netstat -tuln | grep 12345
#define BUF_SIZE 	(DMD_SIZE)

uint8_t img[IMG_SIZE] = {};  				// Ordered as R1 G1 B1 R2 G2 B2 ... in ROW MAJOR FORMAT
uint8_t buf[BUF_SIZE];

char log_msg[1000];


void print_log(void) {
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);
    char time_str[9];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", local_time);
    printf("%s - %s - %s\n", PREFIX, time_str, log_msg);
}

void buf_to_img(uint8_t* buf, uint8_t* img, int buf_size) {
	for (int i=0, j=0; j<buf_size; i+=3, j++) {
		img[i+0] = buf[j];
		img[i+1] = buf[j];
		img[i+2] = buf[j];
	}
}

void gen_rand_img(unsigned char* img, int width, int height) {
    srand(time(NULL)); // Seed the random number generator with the current time

    for (int i = 0; i < width * height * 3; i++) {
        img[i] = rand() % 256; // Generate a random value between 0 and 255
    }
}

int read_image(int newsockfd, int sockfd) {
	int bytes_in = 0;
	int total_bytes_in = 0;
	while (total_bytes_in < DMD_SIZE * sizeof(uint8_t)) {
		bytes_in = read(newsockfd, buf, BUF_SIZE);
		if (bytes_in < 0) {
			perror("ERROR reading from socket");
			sprintf(log_msg, "ERROR reading from socket");
			print_log();
			return -1;
		} else if (bytes_in == 0) {
			sprintf(log_msg, "WARNING Connection closed by the client. Shutting down...\n");
			print_log();
			return 1;
		}
		buf_to_img(buf, img+total_bytes_in*3, bytes_in);
		total_bytes_in += bytes_in;
	}
	return 0;
}

int run_connection_test(int newsockfd, int sockfd) {
	int test_passed = 0;
	int has_printed = 0;
	sprintf(log_msg, "Starting enumeration test...");
	print_log();
	if (read_image(newsockfd, sockfd) != 0) {
		test_passed = -1;
	}
    for (int i=0; i<DMD_WIDTH; i++) {
    	for (int j=0; j<DMD_HEIGHT*3; j+= 3) {
    		int val = i % 255;
    		const int shift = i*DMD_HEIGHT*3;
    		if ((img[j+shift] != val) | (img[j+1+shift] != val) | (img[j+2+shift] != val)) {
				if (has_printed == 0) {
					sprintf(log_msg, "Expected %d at (%d, %d). Received img=(%d,%d,%d)",
							val, i, j/3, img[j+shift], img[j+1+shift], img[j+2+shift]);
					print_log();
					has_printed = 1;
				}
				test_passed -= 1;
			}
    	}
    }
    if (test_passed == 0) {
		sprintf(log_msg, "Connection test passed.");
		print_log();
		for (int i=0; i<IMG_SIZE; i++) {
			img[i] = INIT_VALUE;
		}
    }
    return test_passed;
}


int main(int argc, char* argv[]) {
	int iter = 0;
    int running = 1;
	// Socket
    int sockfd;
    int newsockfd;
    unsigned int clilen;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;

	// SDL
    int numDisplays;
    SDL_DisplayID* displayIDs;
	SDL_Rect rect;
	SDL_Renderer* renderer;
	SDL_Texture* texture;
    SDL_Event e;
    SDL_Window* window;

    sprintf(log_msg, "Initialising SDL...");
    print_log();
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
	    sprintf(log_msg, "SDL initialisation failed: %s\n", SDL_GetError());
		print_log();
        return -1;
    }

	displayIDs = SDL_GetDisplays(&numDisplays);
	if (numDisplays < 2) {
	    sprintf(log_msg, "Not enough displays available. Found %d displays.", numDisplays);
		print_log();
		SDL_Quit();
		return -1;
	}

	SDL_GetDisplayBounds(displayIDs[1], &rect);
    window = SDL_CreateWindowWithPosition("Evolution Machine DMD", rect.x, rect.y, DMD_WIDTH, DMD_HEIGHT, SDL_WINDOW_BORDERLESS);
    sprintf(log_msg, "Using display %d at x=%d and y=%d, of size w=%d, h=%d\n", 1, rect.x, rect.y, rect.w, rect.h);
	print_log();

    if (window == NULL) {
	    sprintf(log_msg, "Window creation failed: %s\n", SDL_GetError());
        print_log();
        SDL_Quit();
        return -1;
    }
    SDL_SetWindowAlwaysOnTop(window, SDL_TRUE);

    renderer = SDL_CreateRenderer(window, NULL, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
		SDL_DestroyRenderer(renderer);
	    sprintf(log_msg, "Renderer creation failed: %s\n", SDL_GetError());
        print_log();
        SDL_Quit();
        return -1;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, rect.w, rect.h);
	if (texture == NULL) {
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
	    sprintf(log_msg, "SDL_CreateTexture Error: %s\n", SDL_GetError());
        print_log();
		SDL_Quit();
		return -1;
	}
	while (SDL_PollEvent(&e) != 0) {
		if (e.type == SDL_EVENT_QUIT) {
			running = 0;
		}
	}
	// SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	for (int i=0; i<IMG_SIZE; i++) {
		img[i] = INIT_VALUE;
	}
	SDL_RenderClear(renderer);
	SDL_UpdateTexture(texture, NULL, img, rect.w * 3);
	SDL_RenderTexture(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);

	// Socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        sprintf(log_msg, "ERROR opening socket");
		print_log();
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        sprintf(log_msg, "ERROR on binding");
		print_log();
		return -1;
    }

    sprintf(log_msg, "Waiting for incoming connection...");
	print_log();
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); // Blocking
    if (newsockfd < 0) {
        perror("ERROR on accept");
        sprintf(log_msg, "ERROR on accept");
		print_log();
		return -1;
    }
    if (run_connection_test(newsockfd, sockfd) != 0) {
        sprintf(log_msg, "ERROR on connection test");
		print_log();
		return -1;

    }
	sprintf(log_msg, "Initialised. Starting event loop...");
	print_log();
    while (running) {
		sprintf(log_msg, "At image %d.\n", iter);
		print_log();
    	if (read_image(newsockfd, sockfd) != 0) {
    		break;
    	}
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		SDL_UpdateTexture(texture, NULL, img, rect.w * 3);
		SDL_RenderTexture(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
		sprintf(log_msg, "Image transmission complete.");
		print_log();
        iter++;
    }

    close(newsockfd);
    close(sockfd);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_free(displayIDs);
    SDL_Quit();

    return 0;
}
