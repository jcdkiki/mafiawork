/* ********************************************** */
/* ALL OS DEPENDENT THINGS SHOULD BE LOCATED HERE */
/* MAYBE?                                         */
/* ********************************************** */

#include "def.h"

#include <handleapi.h>
#include <processthreadsapi.h>
#include <stdint.h>
#include <stdio.h>
#include <hidusage.h>
#include <stdarg.h>
#include <winnt.h>
#include <psapi.h>

#include "sys.h"
#include "audio.h"
#include "game.h"
#include "gfx.h"
#include "entity.h"
#include "utils.h"
#include "rwstream.h"

typedef struct {
    HWND hwnd;
    HGLRC glcontext;
    HDC hdc;
    const wchar_t *window_class_name;
} WinApiData;

enum {
    KEY_RELEASED = 0,
    KEY_PRESSED = 1
};

sys_common_t sys;
WinApiData winapi = {
    .window_class_name = "reng"
};

uint8_t keymap[256];
uint64_t keytick[256];

#ifdef RENG_ENABLE_LOG
FILE *logfile;

void sys_logf(const char *fmt, const char *file, int line, ...)
{
    SYSTEMTIME st;
    
    va_list args;
    va_start(args, line);

    GetLocalTime(&st);

    fprintf(logfile, "INFO %02d/%02d/%d %02d:%02d:%02d [%s:%d]: ", st.wDay, st.wMonth + 1, st.wYear, st.wHour, st.wMinute, st.wSecond, file, line);
    vfprintf(logfile, fmt, args);

    fflush(logfile);
    va_end(args);
}

void sys_log(const char *str, const char *file, int line)
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    fprintf(logfile, "INFO %02d/%02d/%d %02d:%02d:%02d [%s:%d]: ", st.wDay, st.wMonth + 1, st.wYear, st.wHour, st.wMinute, st.wSecond, file, line);
    fprintf(logfile, str);              // is it ok? should i do fprintf("%s", str)?
    
    fflush(logfile);
}

#endif

#ifdef RENG_MEMTRACE
#define ALLOCSTACK_SIZE 8
FILE* memfile;

typedef struct alloc_info {
    void *ptr;
    const char *file;
    int line;
    uint64_t time;
} alloc_info_t;

alloc_info_t recentmem[ALLOCSTACK_SIZE];

uint64_t n_allocs, n_reallocs, n_frees;

void *sys_internal_malloc(size_t size, const char *file, int line)
{
    n_allocs++;
        
    uint64_t min_time = recentmem[0].time;
    int min_i = 0;
    
    for (int i = 1; i < ALLOCSTACK_SIZE; i++) {
        if (recentmem[i].time < min_time) {
            min_time = recentmem[i].time;
            min_i = i;
        }
    }

    if (min_time)
        fprintf(memfile, "M %llu %s %d\n", recentmem[min_i].ptr, recentmem[min_i].file, recentmem[min_i].line);

    recentmem[min_i].ptr = malloc(size);
    recentmem[min_i].time = time(NULL);
    recentmem[min_i].file = file;
    recentmem[min_i].line = line;
    return recentmem[min_i].ptr;
}

void *sys_internal_realloc(void *mem, size_t newsize, const char *file, int line)
{
    n_reallocs++;
    if (!mem) {
        n_allocs--;
        return sys_internal_malloc(newsize, file, line);
    }
    else {
        for (int i = 0; i < ALLOCSTACK_SIZE; i++) {
            if (recentmem[i].ptr == mem) {
                recentmem[i].file = file;
                recentmem[i].line = line;
                recentmem[i].ptr = realloc(mem, newsize);
                recentmem[i].time = time(NULL);
                return recentmem[i].ptr;
            }
        }

        void *res = realloc(mem, newsize);
        fprintf(memfile, "R %llu %llu %s %d\n", res, mem, file, line);
        return res;
    }
}

void sys_internal_free(void *mem, const char *file, int line)
{
    if (mem) {
        for (int i = 0; i < ALLOCSTACK_SIZE; i++) {
            if (recentmem[i].ptr == mem) {
                recentmem[i].ptr = NULL;
                recentmem[i].time = 0;
                goto skip;
            }
        }

        fprintf(memfile, "F %llu %s %d\n", mem, file, line);

    skip:
        n_frees++;
        free(mem);
    }
}

struct {
    uint64_t genbuf, delbuf;
    uint64_t gentx, deltx;
    uint64_t genva, delva;
} glcnt;

void gl_internal_gen_buffers(GLsizei n, GLuint *ptr, const char *file, int line)
{
    glcnt.genbuf += n;
    glGenBuffers(n, ptr);
    for (int i = 0; i < n; i++)
        fprintf(memfile, "GB %u %s %d\n", ptr[i], file, line);
}

void gl_internal_gen_textures(GLsizei n, GLuint *ptr, const char *file, int line)
{
    glcnt.gentx += n;
    glGenTextures(n, ptr);
    for (int i = 0; i < n; i++)
        fprintf(memfile, "GT %u %s %d\n", ptr[i], file, line);
}

void gl_internal_gen_vertex_arrays(GLsizei n, GLuint *ptr, const char *file, int line)
{
    glcnt.genva += n;
    glGenVertexArrays(n, ptr);
    for (int i = 0; i < n; i++)
        fprintf(memfile, "GVA %u %s %d\n", ptr[i], file, line);
}

void gl_internal_delete_buffers(GLsizei n, GLuint *ptr, const char *file, int line)
{
    glcnt.delbuf += n;
    for (int i = 0; i < n; i++)
        fprintf(memfile, "DB %u %s %d\n", ptr[i], file, line);
    glDeleteBuffers(n, ptr);
}

void gl_internal_delete_textures(GLsizei n, GLuint *ptr, const char *file, int line)
{
    glcnt.deltx += n;
    for (int i = 0; i < n; i++)
        fprintf(memfile, "DT %u %s %d\n", ptr[i], file, line);
    glDeleteTextures(n, ptr);
}

void gl_internal_delete_vertex_arrays(GLsizei n, GLuint *ptr, const char *file, int line)
{
    glcnt.delva += n;
    for (int i = 0; i < n; i++)
        fprintf(memfile, "DVA %u %s %d\n", ptr[i], file, line);
    glDeleteVertexArrays(n, ptr);
}

#endif

void sys_close_window()
{
    DestroyWindow(winapi.hwnd);
}

int keyup_size = 0;
uint8_t keyup_stack[256];

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        
        case WM_DESTROY:
            sys.running = false;
            PostQuitMessage(0);
            break;
        
        case WM_KEYDOWN:
            keymap[wParam] = KEY_PRESSED;
            keytick[wParam] = sys.tick;
            game_key_down(wParam);
            break;
        
        case WM_KEYUP:
            keyup_stack[keyup_size++] = wParam;
            break;

        case WM_SIZE:
            sys.width = LOWORD(lParam);
            sys.height = HIWORD(lParam);
            break;

        case WM_INPUT: 
            {
                UINT dwSize = sizeof(RAWINPUT);
                static BYTE lpb[sizeof(RAWINPUT)];

                GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));
                RAWINPUT* raw = (RAWINPUT*)lpb;

                if (raw->header.dwType == RIM_TYPEMOUSE) {
                    sys.mouse_delta.x += raw->data.mouse.lLastX;
                    sys.mouse_delta.y += raw->data.mouse.lLastY;
                } 
                break;
            }

        case WM_PAINT:
            {
                glViewport(0, 0, sys.width, sys.height);
                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();

                PAINTSTRUCT ps;
                BeginPaint(hwnd, &ps);
                game_draw();
                EndPaint(hwnd, &ps);
            }
            ValidateRect(hwnd, NULL);
            SwapBuffers(winapi.hdc);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int64_t GetTime()
{
    return GetTickCount64();
}

void attach_gl()
{
    PIXELFORMATDESCRIPTOR descriptor;

    descriptor.nSize = sizeof(descriptor);
    descriptor.nVersion = 1;
    descriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP | PFD_SUPPORT_OPENGL | PFD_GENERIC_ACCELERATED | PFD_DOUBLEBUFFER | PFD_SWAP_LAYER_BUFFERS;
    descriptor.iPixelType = PFD_TYPE_RGBA;
    descriptor.cColorBits = 32;
    descriptor.cRedBits = 8;
    descriptor.cGreenBits = 8;
    descriptor.cBlueBits = 8;
    descriptor.cAlphaBits = 8;
    descriptor.cDepthBits = 32;
    descriptor.cStencilBits = 8;

    int pixel_format = ChoosePixelFormat(winapi.hdc, &descriptor);
    SetPixelFormat(winapi.hdc, pixel_format, &descriptor);

    HGLRC glcontext = wglCreateContext(winapi.hdc);
    wglMakeCurrent(winapi.hdc, glcontext);
}

int sys_is_key_pressed(int key)
{
    return keymap[key] == KEY_PRESSED;
}

int sys_is_key_just_pressed(int key)
{
    return keymap[key] == KEY_PRESSED && keytick[key] == sys.tick;
}

int sys_is_key_released(int key)
{
    return keymap[key] == KEY_RELEASED && keytick[key] == sys.tick;
}

void sys_fatal_error(const char* msg)
{
    RENG_LOGF("Fatal error with message: %s", msg);
    RENG_LOGF("Exiting with code 1");
    exit(1);
}

file_handle_t sys_open_file(const char* name, const char* openflags)
{
    file_handle_t res;
    if (fopen_s(&res, name, openflags) != 0) {
        sys_fatal_error("Failed to open file");
    }
    return res;
}

file_handle_t sys_reopen_file(file_handle_t file, const char* name, const char* openflags)
{
    if (freopen_s(&file, name, openflags, file) != 0) {
        sys_fatal_error("Failed to reopen file");
    }
    return file;
}

void sys_close_file(file_handle_t file)
{
    if (fclose(file) != 0) {
        sys_fatal_error("Failed to close file");
    }
}

size_t sys_read_file(file_handle_t file, void* dst, size_t bytes)
{
    return fread(dst, 1, bytes, file);
}

size_t sys_get_file_pos(file_handle_t file)
{
    return ftell(file);
}

void sys_set_file_pos(file_handle_t file, size_t offset, FILEPOS type)
{
    static uint32_t map[2] = {
        [FILEPOS_SET] = SEEK_SET,
        [FILEPOS_ADD] = SEEK_CUR
    };

    fseek(file, offset, map[type]);
}


int main()
{
    #ifdef RENG_ENABLE_LOG
    fopen_s(&logfile, "log.txt", "w");
    #endif

    #ifdef RENG_MEMTRACE
    fopen_s(&memfile, "memtrace.txt", "w");
    #endif

    #ifdef DEBUG
    RENG_LOG("BUILT IN DEBUG MODE");
    #endif

    WNDCLASSEX wc;
    MSG msg;

    HINSTANCE hinst = (HINSTANCE)GetModuleHandle(NULL);

    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hinst;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = winapi.window_class_name;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, L"RegisterClassEx Failed", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    sys.width = 640;
    sys.height = 480;

    RECT wr = { 0, 0, sys.width, sys.height };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    winapi.hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        winapi.window_class_name,
        L"reng",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, wr.right - wr.left, wr.bottom - wr.top,
        NULL, NULL, hinst, NULL
    );

    if (!winapi.hwnd) {
        RENG_LOG("CreateWindowEx Failed");
        return 0;
    }

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());

    winapi.hdc = GetDC(winapi.hwnd);
    attach_gl();

    RAWINPUTDEVICE Rid[1];
    Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC; 
    Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE; 
    Rid[0].dwFlags = RIDEV_INPUTSINK;   
    Rid[0].hwndTarget = winapi.hwnd;
    RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]));
    ShowCursor(false);

    audio_init();
    gfx_init();
    rw_init();
    game_init();

    ShowWindow(winapi.hwnd, 1);
    UpdateWindow(winapi.hwnd);

    int64_t next_game_tick = sys.time = GetTime();
    sys.running = true;

    while (sys.running) {
        POINT mp;
        GetCursorPos(&mp);
        ScreenToClient(winapi.hwnd, &mp);
        sys.mouse = VEC2F(mp.x, mp.y);

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        int loops = 0;
        while ((sys.time = GetTime()) > next_game_tick && loops < MAX_FRAMESKIP) {
            PROCESS_MEMORY_COUNTERS mc;
            mc.cb = sizeof(mc);
            GetProcessMemoryInfo(hProcess, &mc, sizeof(mc));
            sys.mem_usage = mc.WorkingSetSize;

            game_tick();
            sys.mouse_delta = VEC2F(0, 0);

            for (int i = 0; i < keyup_size; i++) {
                keymap[keyup_stack[i]] = KEY_RELEASED;
                keytick[keyup_stack[i]] = sys.tick;
                game_key_up(keyup_stack[i]);
            }
            keyup_size = 0;

            next_game_tick += SKIP_TICKS;
            loops++;
            sys.tick++;
        }

        int s = SKIP_TICKS - next_game_tick + sys.time;
        sys.interpolation = (float)s / SKIP_TICKS;

        InvalidateRect(winapi.hwnd, NULL, 0);
        Sleep(0);
    }

    game_deinit();
    rw_deinit();
    gfx_deinit();
    audio_deinit();

    CloseHandle(hProcess);

    #ifdef RENG_MEMTRACE
    for (int i = 0; i < ALLOCSTACK_SIZE; i++)
        if (recentmem[i].ptr) fprintf(memfile, "M %llu %s %d\n", recentmem[i].ptr, recentmem[i].file, recentmem[i].line);

    fprintf(memfile, "END\n"
           "malloc %llu\n"
           "realloc %llu\n"
           "free %llu\n"
           "glGenBuffers %llu\n"
           "glGenTextures %llu\n"
           "glGenVertexArrays %llu\n"
           "glDeleteBuffers %llu\n"
           "glDeleteTextures %llu\n"
           "glDeleteVertexArrays %llu\n"
           ,
           n_allocs, n_reallocs, n_frees,
           glcnt.genbuf, glcnt.gentx, glcnt.genva,
           glcnt.delbuf, glcnt.deltx, glcnt.delva
    );
    
    fclose(memfile);
    #endif

    #ifdef RENG_ENABLE_LOG
    fclose(logfile);
    #endif
    
    return 0;
}
