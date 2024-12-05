/*
carousel - display a gallery of pictures one at a time and switch to other sets
https://github.com/jzu/carousel

0 starts the show, [123456789] switches to another series of pictures (static)
Needs at least a ./0 subdirectory, the main series, optionally ./1, ./2, ./3...
Left / right arrow keys to go to previous / next picture
PgUp / PgDn to increment / decrement by 50 the current picture index
An optional numerical argument gives a different delay between transitions
Receives UDP frames from a remote keyboard, converted to events
Compiling: gcc carousel.c -g -o carousel -lSDL2 -lSDL2_image -lpthread -Wall
*/

#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_video.h>

#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 

// Seconds between transitions
#define DELAY    6
// Intro and outro
#define BLACK    "/tmp/Black.bmp"
// Max because only one keystroke allowed
#define P_SERIES 10
// Max number of filenames in a series
#define P_NUMBER 1000
// Max filename size
#define P_SIZE   64
// PgUp / PgDn
#define SKIP     50
// UDP 
#define PORT     1234 
// For remote keyboard events
#define MAXLINE  1024 
// Debugging
#define LOG "/var/tmp/carousel.log"
// Crossfate increment step (0 → 255)
#define INC 20

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

SDL_Surface* surface = NULL;
SDL_Surface* new_surface = NULL;

SDL_Event l_event,                   // Local event
          r_event;                   // Remote event

pthread_t rthread;                   // Remote keyboard thread

bool running = false;

int series = 0;
int WinW,
    WinH;

char pictures[P_SERIES][P_NUMBER][P_SIZE];
int pidx[P_SERIES];
int pmax[P_SERIES];

time_t t;
struct tm *dthr;
char ymdhms[32];
FILE *Log;


/*
 * list_pictures()
 * If pictures.lst is present in one of several places, use it
 * Else, just use what's in the directories 
 */

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
                                     && (line[0] != '\r')) {
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
                while ((dent = readdir (d)) != 0) {
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


/*
 * transition()
 * Smooth crossfade from an image to another
 * Using alpha from 0 to 255 and vice versa
 */

void transition (SDL_Surface *prv_surface_orig, SDL_Surface *nxt_surface_orig) {

    int a = 0;
    int b = 255;

    SDL_Rect prv_rect;
    SDL_Rect nxt_rect;

    SDL_Surface* prv_surface_copy = NULL;
    SDL_Surface* nxt_surface_copy = NULL;

    SDL_Texture* prv_texture = NULL;
    SDL_Texture* nxt_texture = NULL;
    prv_surface_copy = SDL_CreateRGBSurface (0, WinW, WinH, 32, 0, 0, 0, 0);
    nxt_surface_copy = SDL_CreateRGBSurface (0, WinW, WinH, 32, 0, 0, 0, 0);

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

    SDL_BlitScaled (prv_surface_orig, NULL, prv_surface_copy, &prv_rect);
    SDL_DestroyTexture (prv_texture);
    prv_texture = SDL_CreateTextureFromSurface (renderer, prv_surface_copy);

    SDL_BlitScaled (nxt_surface_orig, NULL, nxt_surface_copy, &nxt_rect);
    SDL_DestroyTexture (nxt_texture);
    nxt_texture = SDL_CreateTextureFromSurface (renderer, nxt_surface_copy);

    while (a < 256) {

        SDL_SetTextureBlendMode (prv_texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureBlendMode (nxt_texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod (prv_texture, b);
        SDL_SetTextureAlphaMod (nxt_texture, a);

        SDL_RenderClear (renderer);
        SDL_RenderCopy (renderer, prv_texture, NULL, NULL);
        SDL_RenderCopy (renderer, nxt_texture, NULL, NULL);
        SDL_SetRenderTarget (renderer, NULL);
        SDL_RenderPresent (renderer);

        a = (a < 256-INC) ? a + INC : 256; 
        b = (b > INC)     ? b - INC : 0;
    }

    SDL_DestroyTexture (prv_texture);
    SDL_DestroyTexture (nxt_texture);

    SDL_FreeSurface (prv_surface_copy);
    SDL_FreeSurface (nxt_surface_copy);
}


/*
 * show_image()
 * Prepares a new surface, which will replace the current one
 * Launches the crossfade
 */

void show_image (char *filename) {

    FILE *fp = fopen ("current.txt", "w");
    fputs (filename, fp);
    fputs ("\n", fp); 
    fclose (fp);

    new_surface = IMG_Load (filename);
    SDL_SetSurfaceBlendMode (new_surface, SDL_BLENDMODE_BLEND);

    transition (surface, new_surface);

    SDL_FreeSurface (surface);
    surface = new_surface;
}


/*
 * prev_image()
 * Takes care of boundaries and rolls over if needed
 */

void prev_image () {

    char path[66];

    if (--pidx[series] < 0)
        pidx[series] = pmax[series]-1;
    sprintf (path, "%d/%s", series, pictures[series][pidx[series]]);
    show_image (path);
}


/*
 * next_image()
 * Takes care of boundaries and rolls over if needed
 */

void next_image () {

    char path[66];

    if (++pidx[series] >= pmax[series])
        pidx[series] = 0;
    sprintf (path, "%d/%s", series, pictures[series][pidx[series]]);
    show_image (path);
}


/*
 * change_series()
 * Switches to another series in response to a numeric key
 * 0 (zero) does not use this function
 */

void change_series (int s) {

    char path[66];

    if (pictures[s][0][0] != 0x00) {
        series = s;
        sprintf (path, "%d/%s", series, pictures[series][pidx[series]]);
        show_image (path);
    }
}


/*
 * remote()
 * Remote keyboard thread
 * Receives chars from network (UDP 1234)
 * Pushes them as events to be processed by the main loop
 * Processes FR non-shifted upper digit keys like digits, using fallthrough
 */

void *remote () {

    unsigned char buffer[MAXLINE]; 
    unsigned char c3 = false;
    int keypad = 0;               // For multi-char keys (tristate 0 1 2)
    int sockfd; 
    socklen_t len;
    struct sockaddr_in servaddr, 
                       cliaddr; 

    if ((sockfd = socket (AF_INET, SOCK_DGRAM, 0)) < 0) { 
        perror ("socket creation failed"); 
        exit (EXIT_FAILURE); 
    } 

    memset (&servaddr, 0, sizeof (servaddr)); 
    memset (&cliaddr, 0, sizeof (cliaddr)); 
    len = sizeof (cliaddr);
       
    servaddr.sin_family      = AF_INET; 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port        = htons (PORT); 
       
    if (bind (sockfd, 
              (const struct sockaddr *) &servaddr,  
              sizeof (servaddr)
       ) < 0) { 
        perror ("bind failed"); 
        exit (EXIT_FAILURE); 
    } 

    r_event.key.windowID = 0;
    r_event.key.state = 1;
    r_event.key.repeat = 0;
    r_event.type = SDL_KEYDOWN;

    while (1) {

        recvfrom (sockfd, 
                  (char *) buffer, 
                  MAXLINE,  
                  MSG_WAITALL, 
                  (struct sockaddr *) &cliaddr, 
                  &len); 
        t = time (0);
        dthr = gmtime (&t);
        strftime (ymdhms, sizeof (ymdhms), "%Y-%m-%dT%H:%M:%SZ", dthr);
        fprintf (Log, "%s [Remote] '%c' (0x%02x)\n", 
                      ymdhms, 
                      (buffer[0] == 0x1b) ? '^' : buffer[0],
                      buffer[0]); 
        switch (buffer[0]) {
            case '[':
                keypad = 1;
                break;
            case 0xc3:
                c3 = true;
                break;
            case 0xa0:
                if (c3) c3 = false; 
                else break;
            case '0':
                r_event.key.keysym.sym = SDLK_0;
                running = true;
                SDL_PushEvent (&r_event);
                break;
            case '&':
            case '1':
                r_event.key.keysym.sym = SDLK_1;
                SDL_PushEvent (&r_event);
                break;
            case 0xa9:
                if (c3) c3 = false; 
                else break;
            case '2':
                r_event.key.keysym.sym = SDLK_2;
                SDL_PushEvent (&r_event);
                break;
            case '"':
            case '3':
                r_event.key.keysym.sym = SDLK_3;
                SDL_PushEvent (&r_event);
                break;
            case '\'':
            case '4':
                r_event.key.keysym.sym = SDLK_4;
                SDL_PushEvent (&r_event);
                break;
            case '(':
            case '5':
                if (keypad == 2) {
                    r_event.key.keysym.sym = SDLK_PAGEUP;
                    SDL_PushEvent (&r_event);
                }
                else {
                    r_event.key.keysym.sym = SDLK_5;
                    SDL_PushEvent (&r_event);
                }
                break;
            case '-':
            case '6':
                if (keypad == 2) {
                    r_event.key.keysym.sym = SDLK_PAGEDOWN;
                    SDL_PushEvent (&r_event);
                }
                else {
                    r_event.key.keysym.sym = SDLK_6;
                    SDL_PushEvent (&r_event);
                }
                break;
            case 0xa8:
                if (c3) c3 = false; 
                else break;
            case '7':
                r_event.key.keysym.sym = SDLK_7;
                SDL_PushEvent (&r_event);
                break;
            case '_':
            case '8':
                r_event.key.keysym.sym = SDLK_8;
                SDL_PushEvent (&r_event);
                break;
            case 0xa7:
                if (c3) c3 = false; 
                else break;
            case '9':
                r_event.key.keysym.sym = SDLK_9;
                SDL_PushEvent (&r_event);
                break;
            case 'C':
                if (keypad == 2) {
                    r_event.key.keysym.sym = SDLK_RIGHT;
                    SDL_PushEvent (&r_event);
                }
                break;
            case 'D':
                if (keypad == 2) {
                    r_event.key.keysym.sym = SDLK_LEFT;
                    SDL_PushEvent (&r_event);
                }
                break;
            case 0x1b:
                r_event.key.keysym.sym = SDLK_ESCAPE;
                SDL_PushEvent (&r_event);
                break;
            case 'q':
                r_event.key.keysym.sym = SDLK_q;
                SDL_PushEvent (&r_event);
                break;

        }
        if (keypad == 2)
            keypad = 0;
        if (keypad == 1)
            keypad = 2;
    }
    return 0; 

}


/*
 * main()
 * SDL initialisation, arrays initialisation
 * (remainders of fake full screen setup -- we'll see)
 * Remote keyboard thread initialisation
 * First picture is a still fading in from a black image
 * waiting for the "0" key
 * Event loop and picture sequence
 * Fade to black on "q" or Escape
 * SDL cleanup
 */

int main (int argc, char *argv[]) {

    SDL_DisplayMode displaymode;
    char path[66];
    int  escape = 0;               // [Esc]-q exits (tristate 0 1 2)
    int  delay = DELAY;


   time (&t);
   Log = fopen (LOG, "a"); 

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
                               SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS |
    SDL_WINDOW_ALWAYS_ON_TOP);
    SDL_RaiseWindow (window);
    renderer = SDL_CreateRenderer (window, 
                                   -1, 
                                   SDL_RENDERER_ACCELERATED |
                                   SDL_RENDERER_PRESENTVSYNC);
    SDL_SetWindowBordered (window, SDL_FALSE);
    SDL_SetWindowFullscreen (window, SDL_WINDOW_FULLSCREEN);
    SDL_SetHint (SDL_HINT_RENDER_SCALE_QUALITY, "linear");  
    SDL_SetHint (SDL_HINT_RENDER_SCALE_QUALITY, "2");  

    surface = IMG_Load (BLACK);

    SDL_SetSurfaceBlendMode (surface, SDL_BLENDMODE_BLEND);

    sprintf (path, "%d/%s", series, pictures[0][0]);
    show_image (path);

    pthread_create (&rthread, NULL, remote, NULL);

    while (! running)
        if (SDL_PollEvent (&l_event) > 0)
            if (l_event.type == SDL_KEYDOWN)
                if ((l_event.key.keysym.sym == SDLK_KP_0) || 
                    (l_event.key.keysym.sym == SDLK_0)) {
                    running = true;
                }
    while (running) {
        while (SDL_PollEvent (&l_event) > 0) {
            if (l_event.type == SDL_KEYDOWN) {
                switch (l_event.key.keysym.sym) {
                    case SDLK_0: 
                    case SDLK_KP_0: 
                        series = 0;
                        sprintf (path, "%d/%s", 0, pictures[0][pidx[0]]);
                        show_image (path);
                        break;
                    case SDLK_1: 
                    case SDLK_KP_1: 
                        change_series (1);
                        break;
                    case SDLK_2: 
                    case SDLK_KP_2: 
                        change_series (2);
                        break;
                    case SDLK_3: 
                    case SDLK_KP_3: 
                        change_series (3);
                        break;
                    case SDLK_4: 
                    case SDLK_KP_4: 
                        change_series (4);
                        break;
                    case SDLK_5: 
                    case SDLK_KP_5: 
                        change_series (5);
                        break;
                    case SDLK_6: 
                    case SDLK_KP_6: 
                        change_series (6);
                        break;
                    case SDLK_7: 
                    case SDLK_KP_7: 
                        change_series (7);
                        break;
                    case SDLK_8: 
                    case SDLK_KP_8: 
                        change_series (8);
                        break;
                    case SDLK_9: 
                    case SDLK_KP_9: 
                        change_series (9);
                        break;
                    case SDLK_LEFT: 
                        prev_image ();
                        break;
                    case SDLK_RIGHT: 
                        next_image ();
                        break;
                    case SDLK_PAGEDOWN: 
                        if (series == 0) {
                            pidx[0] = (pidx[0] - SKIP < 0)
                                      ? 0
                                      : pidx[0] - SKIP;
                            sprintf (path, "%d/%s", 0, pictures[0][pidx[0]]);
                            show_image (path);
                        }
                        break;
                    case SDLK_PAGEUP: 
                        if (series == 0) {
                            pidx[0] = (pidx[0] + SKIP) % pmax[0];
                            sprintf (path, "%d/%s", 0, pictures[0][pidx[0]]);
                            show_image (path);
                        }
                        break;
                    case SDLK_ESCAPE: 
                        escape = 1;
                        break;
                    case SDLK_q: 
                        if (escape == 2)
                            running = false;
                        break;
                 }
                 if (escape == 2)
                     escape = 0;
                 if (escape == 1)
                     escape = 2;
            }
        }
        if (series == 0) {
            sleep (delay);
            next_image (pictures[pidx[series]]);
        }
    }

    show_image (BLACK);

    SDL_FreeSurface (surface);

    SDL_DestroyRenderer (renderer);
    SDL_DestroyWindow (window);
    SDL_Quit ();
    fclose (Log); 

    return 0;
}
