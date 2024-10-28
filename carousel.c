/*
gcc carousel.c -g -o carousel -lSDL2 -lSDL2_image && ./carousel
*/


#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_video.h>

#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>

#define DELAY 6
#define BLACK "/tmp/Black.bmp"
#define P_SERIES 6
#define P_NUMBER 1000
#define P_SIZE 64

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

SDL_Surface* surface = NULL;
SDL_Surface* new_surface = NULL;

SDL_Event event;

int series = 0;
int WinW, 
    WinH;

char pictures[P_SERIES][P_NUMBER][P_SIZE];
int pidx[P_SERIES];
int pmax[P_SERIES];


void error_message () {

  if (! opendir ("0"))
    fprintf (stderr, "Needs subdirectories 0 [1 2 3] with pictures\n");
  exit (1);
}


void list_pictures () {

    int i,
        j;
    DIR *d;
    struct dirent *dent;
    int series;
    char dirname[2];

    char lst_name[64];
    char line[64];
    bool file_present = true;
    char cfg1[64],
         cfg2[64];
    FILE *fd;
    struct stat sb;

    const char black[30] = {0x42,0x4d,0x1e,0x00,0x00,0x00,0x00,0x00,
                            0x00,0x00,0x1a,0x00,0x00,0x00,0x0c,0x00,
                            0x00,0x00,0x01,0x00,0x01,0x00,0x01,0x00,
                            0x18,0x00,0x00,0x00,0x00,0x00};

    int blackfile = creat (BLACK, 
                           S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    write (blackfile, &black, 30);
    close (blackfile);

    bzero (pictures, P_SERIES*P_NUMBER*P_SIZE);

    strncpy (cfg1, getenv ("HOME"), 64);
    strncpy (cfg2, getenv ("HOME"), 64);
    strcat (cfg1, "/.carousel/pictures.lst");
    strcat (cfg2, "/.conf/carousel/pictures.lst");

    if (stat ("pictures.lst", &sb) == 0)
        strcpy (lst_name, "pictures.lst");
    else if (stat (cfg1, &sb) == 0)
        strcpy (lst_name, cfg1);
    else if (stat (cfg2, &sb) == 0)
        strcpy (lst_name, cfg2);
    else
        file_present = false;

    if (file_present) {
        i = 0;
        fd = fopen (lst_name, "r");
        while (fgets (line, 64, fd) != NULL) {
            line[strlen (line) - 1] = 0x00;
            if ((strlen (line) != 0) && (line[0] != ' ') 
                                     && (line[0] != '\t') 
                                     && (line[0] != '\r'))
                if (line[1] == ':') {
                    series = (int)line[0] - 48;
                    i = 0;
                }
                else {
                    j = strlen (line) - 1;
                    while ((line[j] == ' ') || 
                           (line[j] == '\t') ||
                           (line[j] == '\r')) {
                        line[j--] = 0x00;
                    }
                    strncpy (pictures[series][i], line, P_SIZE);
                    i++;
                    pmax[series]++;
                }
        }
        fclose (fd);
        series = 0;
    }
    else
        for (series=0; series<P_SERIES; series++) {
            i = 0;
            dirname[0] = 48 + series;
            dirname[1] = 0;
            d = opendir (dirname);
            if (d) {
                while (dent = readdir (d)) {
                    if (dent->d_name[0] != '.') {
                        strncpy (pictures[series][i], dent->d_name, P_SIZE);
                        i++;
                        pmax[series]++;
                    }
                }
                closedir (d);
            }
        }
}


void transition (SDL_Surface *prv_surface_orig, SDL_Surface *nxt_surface_orig) {

    Uint8 a = 0;
    Uint8 b = 250;

    SDL_Rect prv_rect;
    SDL_Rect nxt_rect;

    SDL_Surface* prv_surface_copy = NULL;
    SDL_Surface* nxt_surface_copy = NULL;

    SDL_Texture* prv_texture = NULL;
    SDL_Texture* nxt_texture = NULL;

    while (a <= 240) {

        prv_texture = SDL_CreateTextureFromSurface (renderer, prv_surface_orig);
        nxt_texture = SDL_CreateTextureFromSurface (renderer, nxt_surface_orig);

        SDL_QueryTexture (prv_texture, NULL, NULL, &prv_rect.w, &prv_rect.h);
        SDL_QueryTexture (nxt_texture, NULL, NULL, &nxt_rect.w, &nxt_rect.h);

        if (prv_rect.w*100/prv_rect.h < WinW*100/WinH) {
            prv_rect.w = (prv_rect.w * WinH) / prv_rect.h;
            prv_rect.h = WinH;
        }
        else {
            prv_rect.h = (prv_rect.h * WinW) / prv_rect.w;
            prv_rect.w = WinW;
        }
        prv_rect.x = (WinW - prv_rect.w) / 2;
        prv_rect.y = (WinH - prv_rect.h) / 2;

        if (! nxt_rect.w) 
            nxt_rect.w = 1;
        if (! nxt_rect.h) 
            nxt_rect.h = 1;
        if (nxt_rect.w*100/nxt_rect.h < WinW*100/WinH) {
            nxt_rect.w = (nxt_rect.w * WinH) / nxt_rect.h;
            nxt_rect.h = WinH;
        }
        else {
            nxt_rect.h = (nxt_rect.h * WinW) / nxt_rect.w;
            nxt_rect.w = WinW;
        }
        nxt_rect.x = (WinW - nxt_rect.w) / 2;
        nxt_rect.y = (WinH - nxt_rect.h) / 2;

        prv_surface_copy = SDL_CreateRGBSurface (0, WinW, WinH, 32, 0, 0, 0, 0);
        SDL_BlitScaled (prv_surface_orig, NULL, prv_surface_copy, &prv_rect);
        SDL_DestroyTexture (prv_texture);
        prv_texture = SDL_CreateTextureFromSurface (renderer, prv_surface_copy);

        nxt_surface_copy = SDL_CreateRGBSurface (0, WinW, WinH, 32, 0, 0, 0, 0);
        SDL_BlitScaled (nxt_surface_orig, NULL, nxt_surface_copy, &nxt_rect);
        SDL_DestroyTexture (nxt_texture);
        nxt_texture = SDL_CreateTextureFromSurface (renderer, nxt_surface_copy);

        SDL_SetTextureBlendMode (prv_texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureBlendMode (nxt_texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod (prv_texture, b);
        SDL_SetTextureAlphaMod (nxt_texture, a);

        SDL_RenderClear (renderer);
        SDL_RenderCopy (renderer, prv_texture, NULL, NULL);
        SDL_RenderCopy (renderer, nxt_texture, NULL, NULL);
        SDL_SetRenderTarget (renderer, NULL);
        SDL_RenderPresent (renderer);

        SDL_DestroyTexture (nxt_texture);
        SDL_DestroyTexture (prv_texture);
        SDL_FreeSurface (prv_surface_copy);
        SDL_FreeSurface (nxt_surface_copy);

        a+=10, b-=10;
    }
}


void show_image (char *filename) {

    new_surface = IMG_Load (filename);
    SDL_SetSurfaceBlendMode (new_surface, SDL_BLENDMODE_BLEND);

    transition (surface, new_surface);

    SDL_FreeSurface (surface);
    surface = new_surface;
}


void prev_image () {

    char path[66];

    if (--pidx[series] < 0)
        pidx[series] = pmax[series]-1;
    sprintf (path, "%d/%s", series, pictures[series][pidx[series]]);
    show_image (path);
}


void next_image () {

    char path[66];

    if (++pidx[series] >= pmax[series])
        pidx[series] = 0;
    sprintf (path, "%d/%s", series, pictures[series][pidx[series]]);
    show_image (path);
}


int main (int argc, char *argv[]) {

    SDL_DisplayMode displaymode;
    char path[66];
    bool running = false;
    bool first_pic = true;
    int  delay = DELAY;

    if (argc > 1)
        if ((argv[1][0] >= '0') && (argv[1][0] <= '9'))
            delay = atoi (argv[1]);

    list_pictures();
    bzero (pidx, P_SERIES * sizeof (int));

    SDL_Init (SDL_INIT_VIDEO);

    SDL_GetCurrentDisplayMode (0, &displaymode);
    WinW = displaymode.w;
    WinH = displaymode.h;

    window = SDL_CreateWindow ("Carousel",
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               WinW,
                               WinH,
                               SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer (window, 
                                   -1, 
                                   SDL_RENDERER_ACCELERATED);
    SDL_SetWindowBordered (window, SDL_FALSE);
    SDL_SetWindowFullscreen (window, SDL_WINDOW_FULLSCREEN);
    SDL_SetHint (SDL_HINT_RENDER_SCALE_QUALITY, "linear");  

    surface = IMG_Load (BLACK);

    SDL_SetSurfaceBlendMode (surface, SDL_BLENDMODE_BLEND);

    sprintf (path, "%d/%s", series, pictures[0][0]);
    show_image (path);
    while (! running)
        if ((SDL_PollEvent (&event) > 0) && (event.type == SDL_KEYDOWN) && 
            ((event.key.keysym.sym == SDLK_KP_0) || (event.key.keysym.sym == SDLK_0)))
            running = true;

    while (running) {
        while (SDL_PollEvent (&event) > 0) {
            if (event.type == SDL_KEYDOWN) {
                switch ( event.key.keysym.sym ) {
                    case SDLK_LEFT: 
                        prev_image ();
                        break;
                    case SDLK_RIGHT: 
                        next_image ();
                        break;
                    case SDLK_0: 
                    case SDLK_KP_0: 
                        series = 0;
                        sprintf (path, "%d/%s", series, pictures[series][pidx[series]]);
                        show_image (path);
                        break;
                    case SDLK_1: 
                    case SDLK_KP_1: 
                        if (pictures[1][0][0] != 0x00) {
                            series = 1;
                            sprintf (path, "%d/%s", series, pictures[series][pidx[series]]);
                            show_image (path);
                        }
                        break;
                    case SDLK_2: 
                    case SDLK_KP_2: 
                        if (pictures[2][0][0] != 0x00) {
                            series = 2;
                            sprintf (path, "%d/%s", series, pictures[series][pidx[series]]);
                            show_image (path);
                        }
                        break;
                    case SDLK_3: 
                    case SDLK_KP_3: 
                        if (pictures[3][0][0] != 0x00) {
                            series = 3;
                            sprintf (path, "%d/%s", series, pictures[series][pidx[series]]);
                            show_image (path);
                        }
                        break;
                    case SDLK_ESCAPE: 
                    case SDLK_q: 
                        running = false;
                        break;
                 }
            }
        }
        usleep (100000);
        if ((series == 0) && ((SDL_GetTicks64()/1000) % delay == 0))
            next_image (pictures[pidx[series]]);
    }

    show_image (BLACK);

    SDL_FreeSurface (surface);

    SDL_DestroyRenderer (renderer);
    SDL_DestroyWindow (window);
    SDL_Quit ();
    return 0;
}