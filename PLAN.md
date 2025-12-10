I'll create a comprehensive plan for adding HDMI support and a custom text renderer to NaoKernel. 

Research HDMI and text rendering in kernels

Read [](file:///e%3A/Project/NaoKernel/README.md)

Read [](file:///e%3A/Project/NaoKernel/ARCHITECTURE.md#1-1), lines 1 to 100

Read [](file:///e%3A/Project/NaoKernel/kernel.c#1-1), lines 1 to 50

Read [](file:///e%3A/Project/NaoKernel/output/output.h)

Using "Fetch Web Page"

Fetched 2 resources

Using "Fetch Web Page"

Fetched 2 resources

Using "Fetch Web Page"

Fetched https://wiki.osdev.org/Bootloader_basics

Using "Fetch Web Page"

Fetched 2 resources

Using "Fetch Web Page"

Fetched 2 resources

Using "Fetch Web Page"

Fetched https://wiki.osdev.org/GOP

Excellent! Now let me create a detailed implementation plan based on this research:

## Plan: HDMI Support & Custom Text Renderer

**TL;DR**: Add graphics framebuffer support with custom bitmap font rendering while maintaining VGA text mode fallback. Use VESA/UEFI to get framebuffer from bootloader, render 8×16 bitmap fonts to pixels. Estimated 2-3 KB code + 4 KB font data.

### Steps

1. **Create graphics abstraction layer** in `output/graphics.h` with `GraphicsMode` struct and dual-mode output functions
2. **Implement bitmap font rasterizer** in `output/font.c` with 8×16 character bitmaps (256 chars, 4 KB total)
3. **Add graphics initialization** in kernel.c to detect VESA/UEFI framebuffer and set up graphics mode
4. **Update output system** to detect graphics vs text mode and route `kprint()` calls appropriately
5. **Implement pixel drawing** function (`draw_pixel()`) for 32-bit ARGB framebuffer writing
6. **Add graphics shell output** - Render shell prompt, text, and colors on framebuffer
7. **Maintain text mode fallback** - Keep existing VGA code for systems without graphics support
8. **Bootloader configuration** - Document GRUB/bootloader setup needed for graphics mode

### Further Considerations

1. **VESA vs UEFI** - VESA (INT 0x10) for legacy BIOS, GOP for UEFI; recommend supporting both or just VESA for simplicity
2. **Resolution flexibility** - Support 640×480, 800×600, 1024×768; detect available modes at boot
3. **Color support** - Use 32-bit ARGB for rich colors; map VGA colors (0x07 white) to RGB equivalents
4. **Performance** - Character rendering is ~128 pixel writes; acceptable for shell, consider double-buffering for full-screen updates later


# PROMPT

# Plan: HDMI Support & Custom Text Renderer for NaoKernel

## Executive Summary

Add graphics framebuffer support with custom bitmap font rendering while maintaining VGA text mode fallback. Use VESA/UEFI to get framebuffer from bootloader, render 8×16 bitmap fonts to pixels. Estimated 2-3 KB code + 4 KB font data.

---

## Background: Graphics Architecture

### Current State
- **VGA Text Mode**: 0xB8000, 80×25 text, 32 KB memory, built-in hardware rendering
- **Limitation**: Legacy, deprecated on modern UEFI systems, no graphics capability

### Target State
- **Dual Mode**: Detect graphics capability at boot, use framebuffer if available, fallback to VGA text
- **Graphics Framebuffer**: 32-bit ARGB, firmware-provided address (typically 0xE0000000+)
- **Custom Text Rendering**: Bitmap fonts (8×16), software rasterization to framebuffer
- **Backward Compatible**: Existing VGA code remains unchanged

### Key Facts
1. **No Direct HDMI Access**: Firmware (BIOS/UEFI) handles HDMI negotiation; kernel receives working framebuffer
2. **VESA/UEFI Standard**: Bootloader sets graphics mode before kernel loads via VESA INT 0x10 or UEFI GOP
3. **Safe Resolutions**: 640×480, 800×600, 1024×768 @ 24/32-bit universally supported
4. **Memory**: 1-5 MB for framebuffer + 4 KB font + small overhead = ~5 MB total

---

## Implementation Plan

### Phase 1: Graphics Abstraction Layer

**File**: `output/graphics.h`

```c
#ifndef GRAPHICS_H
#define GRAPHICS_H

/* Graphics mode information from bootloader */
typedef struct {
    uint32_t width;           /* Horizontal resolution in pixels */
    uint32_t height;          /* Vertical resolution in pixels */
    uint32_t pitch;           /* Bytes per scanline */
    uint8_t bpp;              /* Bits per pixel (24 or 32) */
    void* framebuffer;        /* Physical address of framebuffer */
    int available;            /* 1 if graphics mode set, 0 if fallback to VGA */
} GraphicsMode;

/* Cursor state for both text and graphics modes */
typedef struct {
    int col;                  /* Column (0-79 for text, or pixel x for graphics) */
    int row;                  /* Row (0-24 for text, or pixel y for graphics) */
    uint32_t fg_color;        /* Foreground color (ARGB for graphics, ignored for text) */
    uint32_t bg_color;        /* Background color (ARGB for graphics, ignored for text) */
} CursorState;

/* Function pointers for dual-mode output */
extern void (*output_write_char)(char c, int x, int y, uint32_t fg, uint32_t bg);
extern void (*output_clear_screen)(void);
extern void (*output_newline)(void);

/* Graphics mode functions */
void graphics_init(GraphicsMode* mode);
void graphics_write_char(char c, int x, int y, uint32_t fg, uint32_t bg);
void graphics_clear_screen(void);
void graphics_newline(void);
void draw_pixel(uint32_t x, uint32_t y, uint32_t color);

/* Text mode functions (existing, kept for fallback) */
void text_write_char(char c, int x, int y);
void text_clear_screen(void);
void text_newline(void);

#endif /* GRAPHICS_H */
```

### Phase 2: Bitmap Font Implementation

**File**: `output/font.h`

```c
#ifndef FONT_H
#define FONT_H

#include <stdint.h>

#define FONT_WIDTH 8
#define FONT_HEIGHT 16
#define FONT_CHARS 256

/* Bitmap font: 256 characters × 16 bytes each = 4 KB */
extern const uint8_t bitmap_font[FONT_CHARS][FONT_HEIGHT];

/* Render a single character to framebuffer */
void font_render_char(uint32_t* framebuffer, uint32_t pitch,
                      char c, int x, int y, uint32_t fg, uint32_t bg);

#endif /* FONT_H */
```

**File**: `output/font.c`

```c
#include "font.h"

/* Bitmap font data: Each character is 16 bytes (16 rows × 8 bits) */
const uint8_t bitmap_font[FONT_CHARS][FONT_HEIGHT] = {
    /* 0x00: NULL - rendered as empty */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    
    /* 0x01-0x1F: Control characters (empty) */
    /* ... (32 rows of zeros) ... */
    
    /* 0x20: Space */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    
    /* 0x21: ! */
    {0x00, 0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    
    /* ... Continue for all 256 characters ... */
    /* Use standard PC Screen Font (PSF) or VGA BIOS font data */
};

/* Render a character at (x, y) with foreground/background colors */
void font_render_char(uint32_t* framebuffer, uint32_t pitch,
                      char c, int x, int y, uint32_t fg, uint32_t bg)
{
    const uint8_t* glyph = bitmap_font[(unsigned char)c];
    int row, col;
    
    for (row = 0; row < FONT_HEIGHT; row++) {
        uint8_t byte = glyph[row];
        uint32_t* line_ptr = framebuffer + (y + row) * (pitch / 4) + x;
        
        for (col = 0; col < FONT_WIDTH; col++) {
            uint32_t color = (byte & (0x80 >> col)) ? fg : bg;
            line_ptr[col] = color;
        }
    }
}
```

### Phase 3: Graphics Mode Initialization

**File**: `output/graphics.c`

```c
#include "graphics.h"
#include "font.h"

static GraphicsMode current_graphics;
static CursorState cursor;

/* Initialize graphics mode */
void graphics_init(GraphicsMode* mode)
{
    if (!mode || !mode->available) {
        return;
    }
    
    current_graphics = *mode;
    cursor.col = 0;
    cursor.row = 0;
    cursor.fg_color = 0xFFFFFF;  /* White */
    cursor.bg_color = 0x000000;  /* Black */
    
    /* Clear framebuffer */
    graphics_clear_screen();
}

/* Clear entire screen */
void graphics_clear_screen(void)
{
    uint32_t* fb = (uint32_t*)current_graphics.framebuffer;
    uint32_t pitch_words = current_graphics.pitch / 4;
    int i;
    
    for (i = 0; i < current_graphics.height * pitch_words; i++) {
        fb[i] = cursor.bg_color;
    }
    
    cursor.col = 0;
    cursor.row = 0;
}

/* Draw pixel at (x, y) */
void draw_pixel(uint32_t x, uint32_t y, uint32_t color)
{
    if (x >= current_graphics.width || y >= current_graphics.height) {
        return;
    }
    
    uint32_t* fb = (uint32_t*)current_graphics.framebuffer;
    uint32_t offset = y * (current_graphics.pitch / 4) + x;
    fb[offset] = color;
}

/* Write character at current cursor position */
void graphics_write_char(char c, int x, int y, uint32_t fg, uint32_t bg)
{
    font_render_char((uint32_t*)current_graphics.framebuffer,
                     current_graphics.pitch, c, x, y, fg, bg);
}

/* Handle newline in graphics mode */
void graphics_newline(void)
{
    cursor.col = 0;
    cursor.row += FONT_HEIGHT;
    
    /* Scroll if needed */
    if (cursor.row >= current_graphics.height) {
        /* Simple scroll: move all content up by one line */
        /* For now, just wrap around */
        cursor.row = 0;
    }
}
```

### Phase 4: Dual-Mode Output System

**Update**: `output/output.h`

```c
/* Add to output.h */
#include "graphics.h"

/* Initialize output system (text or graphics) */
void output_init(void);

/* Check if graphics available */
int output_has_graphics(void);
```

**Update**: `output/output.c`

```c
#include "graphics.h"
#include "../output/output.h"

static int use_graphics = 0;
static GraphicsMode gfx_mode;

/* Function pointers for dual-mode operation */
void (*output_write_char_func)(char c, int x, int y, uint32_t fg, uint32_t bg);
void (*output_clear_screen_func)(void);

/* Initialize output system */
void output_init(void)
{
    /* Try to detect graphics mode from bootloader */
    if (detect_graphics_mode(&gfx_mode)) {
        graphics_init(&gfx_mode);
        output_write_char_func = graphics_write_char;
        output_clear_screen_func = graphics_clear_screen;
        use_graphics = 1;
    } else {
        /* Fallback to VGA text mode */
        output_write_char_func = text_write_char;
        output_clear_screen_func = text_clear_screen;
        use_graphics = 0;
    }
}

int output_has_graphics(void)
{
    return use_graphics;
}

/* Detect graphics mode from bootloader (VESA/UEFI) */
int detect_graphics_mode(GraphicsMode* mode)
{
    /* TODO: Implement VESA VBE or UEFI GOP detection */
    /* Query framebuffer address, resolution, and bit depth */
    /* For now, return 0 (fallback to VGA) */
    return 0;
}

/* Update kprint to use graphics if available */
void kprint(const char* str)
{
    int x = 0, y = 0;
    
    while (*str) {
        if (*str == '\n') {
            y += (use_graphics ? FONT_HEIGHT : 1);
            x = 0;
        } else {
            output_write_char_func(*str, x, y, 0xFFFFFF, 0x000000);
            x += (use_graphics ? FONT_WIDTH : 1);
        }
        str++;
    }
}
```

### Phase 5: Bootloader Configuration

**Update**: Bootloader configuration (GRUB)

```bash
# In grub.cfg for graphics support
insmod all_video
set gfxmode=800x600x32
insmod gfxterm
terminal_output gfxterm
```

**Alternative**: Custom bootloader needs to:
1. Call VESA INT 0x10 AH=4F02 to set graphics mode before jumping to kernel
2. Pass graphics mode info to kernel via multiboot header or custom structure
3. Ensure framebuffer is identity-mapped in kernel page tables

---

## Integration Points

### In `kernel.c` (Main Kernel)

```c
void kmain(void)
{
    /* Initialize output system first (text or graphics) */
    output_init();
    
    /* Detect if graphics available */
    if (output_has_graphics()) {
        kprint("Graphics mode enabled: ");
        kprint_dec(gfx_mode.width);
        kprint("x");
        kprint_dec(gfx_mode.height);
        kprint(" @ ");
        kprint_dec(gfx_mode.bpp);
        kprint(" bpp\n");
    } else {
        kprint("Using VGA text mode\n");
    }
    
    /* Rest of kernel initialization */
    idt_init();
    kb_init();
    nano_shell();
}
```

---

## Memory Layout

```
Physical Memory:
├── Kernel Code (0x100000 - 0x200000)
├── Kernel Data (0x200000 - 0x300000)
├── Stack (0x300000 - 0x400000)
├── VGA Text Buffer (0xB8000, legacy fallback, 32 KB)
├── Graphics Framebuffer (0xE0000000+, firmware-provided)
│   └── 800×600 @ 32bpp = 1.8 MB
├── Double Buffer (optional, 0xE0200000+, 1.8 MB)
└── Font Data (embedded in kernel, 4 KB)
```

---

## Estimated Code Size & Performance

| Component | Size | Notes |
|-----------|------|-------|
| graphics.c | 1.5 KB | Core graphics functions |
| font.c | 4 KB | Bitmap font data |
| font rendering | ~200 bytes | Per-character rasterization |
| output.c changes | 0.5 KB | Dual-mode integration |
| **Total** | **~6 KB** | Minimal footprint |

### Performance (per character rendered):
- **VGA Text**: 2 bytes write (character + attribute)
- **Graphics**: 128 pixel writes (8×16 bitmap) = ~2 µs on modern CPU
- **Impact**: Acceptable for shell I/O; consider double-buffering for full-screen operations

---

## Implementation Order

1. **Step 1**: Create `graphics.h` abstract interface
2. **Step 2**: Implement `font.c` with bitmap font data
3. **Step 3**: Implement `graphics.c` with core rendering
4. **Step 4**: Add graphics detection in `kernel.c`
5. **Step 5**: Update `output.c` for dual-mode operation
6. **Step 6**: Test with QEMU graphics mode
7. **Step 7**: Configure bootloader for graphics
8. **Step 8** (Future): Add double-buffering and color support to shell

---

## Future Enhancements

1. **UEFI GOP Support**: Detect modern UEFI framebuffer instead of just VESA
2. **Double Buffering**: Allocate back buffer, batch updates, sync with vertical refresh
3. **Scrolling**: Implement proper scrolling instead of wrap-around
4. **Color Support**: Map VGA colors to RGB, add color output to shell
5. **Font Scaling**: Support multiple font sizes (8×16, 16×32, etc.)
6. **Hardware Acceleration**: Use GPU for fills/copies (advanced, likely not needed)

---

## Risk Mitigation

| Risk | Mitigation |
|------|-----------|
| Bootloader doesn't provide framebuffer | Fallback to VGA text mode (already implemented) |
| Framebuffer address changes | Pass via multiboot or query at runtime |
| Resolution variations | Support multiple common resolutions; detect at boot |
| Font data too large | Use compact PSF format; optimize glyph data |
| CPU overhead of rendering | Batch character updates; consider double-buffer sync |

---

## Testing Strategy

1. **QEMU with VGA**: Test fallback mode
2. **QEMU with `-vga std`**: Test graphics mode at 800×600
3. **Physical hardware**: Test on real BIOS/UEFI systems
4. **Bootloader validation**: Ensure GRUB/bootloader passes correct framebuffer info

---

## Notes

- **Framebuffer endianness**: Confirm ARGB vs BGRA on target platform
- **Virtual address mapping**: Framebuffer may need identity mapping or MMU setup
- **Interrupts during rendering**: Ensure keyboard input doesn't corrupt framebuffer state
- **Font copyright**: Use public domain bitmap fonts or create custom 8×16 font
