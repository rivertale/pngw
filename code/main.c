#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIW_WINDOWS_UTF8
#include "stb_image_write.h"
#include <windows.h>

#define MAX_PATH_LEN 65536

static void
fatal_error(char *message)
{
    MessageBoxA(0, message, "error", MB_OK);
    ExitProcess(-1);
}

static int
find_least_significant_bit(int value)
{
    int result = 0;
    for(int i = 0; i < 32; ++i)
    {
        if(value & 1)
            break;
        ++result;
        value >>= 1;
    }
    return result;
}

int
WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int show_cmd)
{
    if(!IsClipboardFormatAvailable(CF_DIB))
        fatal_error("clipboard does not contain an image");

    wchar_t wide_path[MAX_PATH_LEN];
    SYSTEMTIME time;
    GetSystemTime(&time);
    _snwprintf(wide_path, MAX_PATH_LEN, L"%u_%02u_%02u_%02u_%02u_%02u.png",
               time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);

    int extension_offset = -4;
    for(wchar_t *at = wide_path; *at; ++at)
        ++extension_offset;
    
    OPENFILENAMEW path_chooser = {0};
    path_chooser.lStructSize = sizeof(path_chooser);
    path_chooser.lpstrFilter = L"PNG (*.png)\0*.png\0";
    path_chooser.nFilterIndex = 1;
    path_chooser.lpstrFile = wide_path;
    path_chooser.nMaxFile = MAX_PATH_LEN;
    path_chooser.nFileExtension = extension_offset;
    path_chooser.lpstrDefExt = L"png";
    
    char path[MAX_PATH_LEN];
    if(!GetSaveFileNameW(&path_chooser))
    {
        if(CommDlgExtendedError())
            fatal_error("unable to obtain save path");
    }

    if(!WideCharToMultiByte(CP_UTF8, 0, wide_path, -1, path, MAX_PATH_LEN, 0, 0))
        fatal_error("invalid utf-8 conversion");

    if(!OpenClipboard(0))
        fatal_error("unable to open clipboard");

    HANDLE bitmap_handle = GetClipboardData(CF_DIB);
    if(!bitmap_handle)
        fatal_error("unable to obtain clipboard data");
    
    BITMAPINFO *bitmap_info = (BITMAPINFO *)GlobalLock(bitmap_handle);
    if(!bitmap_info)
        fatal_error("unable to lock clipboard bitmap");
    
    BITMAPINFOHEADER *header = &bitmap_info->bmiHeader;
    if(header->biBitCount != 32)
        fatal_error("unsupported bitmap format");

    unsigned int *bitmap = (unsigned int *)(header + 1);
    int width = header->biWidth;
    int height = header->biHeight;
    int mask_a = 0xff000000;
    int mask_r = 0x00ff0000;
    int mask_g = 0x0000ff00;
    int mask_b = 0x000000ff;
    int shift_a = 24;
    int shift_r = 16;
    int shift_g = 8;
    int shift_b = 0;
    if(header->biCompression == BI_BITFIELDS)
    {
        mask_r = bitmap[0];
        mask_g = bitmap[1];
        mask_b = bitmap[2];
        shift_r = find_least_significant_bit(mask_r);
        shift_g = find_least_significant_bit(mask_g);
        shift_b = find_least_significant_bit(mask_b);
        bitmap += 3;
    }

    unsigned int *out_bitmap = (unsigned int *)VirtualAlloc(0, width * height * sizeof(*out_bitmap), MEM_COMMIT, PAGE_READWRITE);
    unsigned int *out_pixel = out_bitmap;
    if(!out_bitmap)
        fatal_error("out of memory");

    unsigned int *row = bitmap + (height - 1) * width;
    for(int y = 0; y < height; ++y)
    {
        unsigned int *pixel = row;
        for(int x = 0; x < width; ++x)
        {
            unsigned int color = *pixel++;
            unsigned char r = (color & mask_r) >> shift_r;
            unsigned char g = (color & mask_g) >> shift_g;
            unsigned char b = (color & mask_b) >> shift_b;
            unsigned char a = (color & mask_a) >> shift_a;
            *out_pixel++ = (a << 24) | (b << 16) | (g << 8) | (r << 0);
        }
        row -= width;
    }

    if(!stbi_write_png(path, width, height, 4, out_bitmap, width * 4))
        fatal_error("unable to save png");

    VirtualFree(out_bitmap, 0, MEM_RELEASE);
    GlobalUnlock(bitmap_handle);
    CloseClipboard();
    return 0;
}
