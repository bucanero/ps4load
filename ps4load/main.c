/*
// PS4Load host - based on the PS3Load tool (PSL1GHT)
//
// compatible with the original ps3load client:
// # export PS3LOAD=tcp:192.168.x.x
// # ps3load /path/to/eboot.bin
*/

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <orbis/libkernel.h>
#include <orbis/Sysmodule.h>
#include <orbis/SystemService.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <zlib.h>
#include <dbglogger.h>
#include <zip.h>

#include "ttf.h"

#define MIN(a, b)               ((a) < (b) ? (a) : (b))
#define SELF_PATH               "/data/ps4load.tmp"
#define VERSION                 "v0.5.0"
#define PORT                    4299
#define MAX_ARG_COUNT           0x100
#define FRAME_WIDTH             1920
#define FRAME_HEIGHT            1080
#define CHUNK                   0x4000
#define PKZIP                   0x04034B50

#define ERROR(a, msg) { \
    if (a < 0) { \
        snprintf(msg_error, sizeof(msg_error), "PS4Load: " msg ); \
        usleep(250); \
        if(my_socket >= 0) { close(my_socket);my_socket = -1;} \
    } \
}
#define ERROR2(a, msg) { \
    if (a < 0) { \
        snprintf(msg_error, sizeof(msg_error), "PS4Load: %s", msg ); \
        usleep(1000000); \
        msg_error[0] = 0; \
        usleep(60); \
        goto reloop; \
    } \
}
#define continueloop() { close(c); goto reloop; }

// Font faces
FT_Face fontFace;

char msg_error[128];
char msg_two  [128];

volatile int my_socket=-1;
volatile int flag_exit=0;

void init_copperbars(void);
void draw_copperbars(SDL_Renderer* render);

void init_sinetext(SDL_Renderer* render, const char* path);
void draw_sinetext(SDL_Renderer* render, int y);


/* Decompress from source data to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
int inflate_data(int source, uint32_t filesize, FILE *dest)
{
    int ret;
    uint32_t have;
    z_stream strm;
    uint8_t src[CHUNK];
    uint8_t out[CHUNK];

    /* allocate inflate state */
    memset(&strm, 0, sizeof(z_stream));
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = MIN(CHUNK, filesize);
        ret = read(source, src, strm.avail_in);
        if (ret < 0) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }

        strm.avail_in = ret;
        if (strm.avail_in == 0)
            break;
        strm.next_in = src;
        filesize -= ret;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            switch (ret) {
            case Z_STREAM_ERROR:
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END && filesize);

    /* clean up and return */
    (void)inflateEnd(&strm);
    return (ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR);
}

int dump_data(int source, uint32_t filesize, FILE *dest)
{
    uint8_t data[CHUNK];
    uint32_t count, pkz = PKZIP;

    while (filesize > 0)
    {
        count = MIN(CHUNK, filesize);
        int ret = read(source, data, count);
        if (ret < 0)
            return Z_DATA_ERROR;

        if (pkz == PKZIP)
            pkz = memcmp(data, &pkz, 4);

        fwrite(data, ret, 1, dest);
        filesize -= ret;
    }

    return (pkz ? Z_OK : PKZIP);
}

int netThread(void* data)
{
    snprintf(msg_two, sizeof(msg_two), "Creating socket...");
    dbglogger_log(msg_two);

    my_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    ERROR(my_socket, "Error creating socket()");

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);
    
    if(my_socket!=-1) {
        snprintf(msg_two, sizeof(msg_two), "Binding socket...");
        dbglogger_log(msg_two);

        ERROR(bind(my_socket, (struct sockaddr*)&server, sizeof(server)), "Error bind()ing socket");
    }

    if(my_socket!=-1)
        ERROR(listen(my_socket, 10), "Error calling listen()");

reloop:
    msg_two[0] = 0;

    while (1) {
        
        usleep(20000);
        
        if(flag_exit) break;
        if(my_socket == -1) continue;

        //fgColor.r = fgColor.g = fgColor.b = 0xff;
        snprintf(msg_two, sizeof(msg_two), "Waiting for connection...");
        dbglogger_log(msg_two);

        int c = accept(my_socket, NULL, NULL);

        if(flag_exit) break;
        if(my_socket == -1) continue;

        ERROR(c, "Error calling accept()");

        uint32_t magic = 0;
        if (read(c, &magic, sizeof(magic)) < 0)
            continueloop();
        if (strncmp((char*)&magic, "HAXX", 4)) {
            snprintf(msg_two, sizeof(msg_two), "Wrong HAXX magic.");
            dbglogger_log(msg_two);
            continueloop();
        }
        if (read(c, &magic, sizeof(magic)) < 0)
            continueloop();
        uint16_t argslen = __bswap32(magic) & 0x0000FFFF;
        
        uint32_t filesize = 0;
        if (read(c, &filesize, sizeof(filesize)) < 0)
            continueloop();

        uint32_t uncompressed = 0;
        if (read(c, &uncompressed, sizeof(uncompressed)) < 0)
            continueloop();

        filesize = __bswap32(filesize);
        uncompressed = __bswap32(uncompressed);

        remove(SELF_PATH);
        FILE *fd = fopen(SELF_PATH, "wb");
        if (!fd) {
            close(c);
            ERROR2(-1, "Error opening temporary file.");
        }

        //fgColor.g = 255;
        snprintf(msg_two, sizeof(msg_two), "Receiving data... (0x%08x/0x%08x)", filesize, uncompressed);
        dbglogger_log(msg_two);

        int ret = uncompressed ? inflate_data(c, filesize, fd) : dump_data(c, filesize, fd);
        fclose(fd);

        if (ret != Z_OK && ret != PKZIP)
            continueloop();

        snprintf(msg_two, sizeof(msg_two), "Receiving arguments... 0x%08x", argslen);
        dbglogger_log(msg_two);

        uint8_t* args = NULL;
        if (argslen) {
            args = (uint8_t*)malloc(argslen);
            if (read(c, args, argslen) < 0)
                continueloop();
        }
        close(c);

        if (!uncompressed && ret == PKZIP)
        {
            snprintf(msg_two, sizeof(msg_two), "Extracting ZIP to /data/ ...");
            dbglogger_log(msg_two);

            ERROR2(zip_extract(SELF_PATH, "/data/", NULL, NULL), "Error extracting ZIP file.");
            goto reloop;
        }

        ERROR2(chmod(SELF_PATH, 0777), "Failed to chmod() temporary file.");

        char* launchargv[MAX_ARG_COUNT];
        memset(launchargv, 0, sizeof(launchargv));

        int i = 0, pos = 0;
        while (pos < argslen) {
            int len = strlen((char*)args + pos);
            if (!len)
                break;
            launchargv[i] = (char*)malloc(len + 1);
            strcpy(launchargv[i], (char*)args + pos);
            pos += len + 1;
            i++;
        }

        snprintf(msg_two, sizeof(msg_two), "Launching...");
        dbglogger_log(msg_two);

        dbglogger_stop();
        sleep(1);

        sceSystemServiceLoadExec(SELF_PATH, (const char**)launchargv);
        sceKernelUsleep(2 * 1000000);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    SDL_Event event;
    SDL_Window *window;
    SDL_Renderer *renderer;
    int done = 0;

    dbglogger_init();

    // mandatory at least on switch, else gfx is not properly closed
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        dbglogger_log("SDL_Init: %s", SDL_GetError());
        return -1;
    }

    /// create a window (OpenGL always enabled)
    /// available switch SDL2 video modes :
    /// 1920 x 1080 @ 32 bpp (SDL_PIXELFORMAT_RGBA8888)
    /// 1280 x 720 @ 32 bpp (SDL_PIXELFORMAT_RGBA8888)
    window = SDL_CreateWindow("sdl2_gles2", 0, 0, FRAME_WIDTH, FRAME_HEIGHT, 0);
    if (!window) {
        dbglogger_log("SDL_CreateWindow: %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // create a renderer (OpenGL ES2)
    renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        dbglogger_log("SDL_CreateRenderer: %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // We have to force load the freetype module or we'll get unresolved NID crashes
    dbglogger_log("Initializing TTF");

    done = sceSysmoduleLoadModule(ORBIS_SYSMODULE_FREETYPE_OL);
    if (done < 0) {
        dbglogger_log("Failed to load freetype module: %X", done);
        return (-1);
    }

    // Finally initialize freetype
    done = FT_Init_FreeType(&ftLib);
    if (done < 0) {
        dbglogger_log("Failed to init freetype lib: %X", done);
        return (-1);
    }

    dbglogger_log("Initializing font");

    // Create a font face for debug and score text
    if (!InitFont(&fontFace, "/app0/assets/fonts/Gontserrat-Regular.ttf", 40)) {
        dbglogger_log("Failed to initialize font");
        return (-1);
    }

    SDL_CreateThread(&netThread, "net", &done);
    init_copperbars();
    init_sinetext(renderer, "/app0/assets/images/psl1ght.tga");

    Color fgColor = {0xFF, 0xFF, 0xFF};

    while (!done)
    {
        SDL_SetRenderDrawColor(renderer, 60, 60, 60, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);

        draw_copperbars(renderer);
        draw_sinetext(renderer, 520);

        DrawString(renderer, "PS4Load " VERSION " by Bucanero", fontFace, fgColor, 0, 100);
        DrawString(renderer, msg_two, fontFace, fgColor, 0, 800);

        if (msg_error[0]) {
            Color errorColor = {0xFF, 0, 0};
            DrawString(renderer, msg_error, fontFace, errorColor, 0, 300);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    dbglogger_stop();

    sceSystemServiceLoadExec("exit", NULL);
    return 0;
}
