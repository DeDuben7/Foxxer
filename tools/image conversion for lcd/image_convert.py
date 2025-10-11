#!/usr/bin/env python3
"""
Convert a PNG image (including 32-bit with alpha) to a 16-bit uncompressed RGB565 BMP
(for use with ST7735/ST7789 OLED/TFT displays and SEGGER tools).

- Pixels with alpha below a threshold (default 20) are treated as fully black.
- Pixels with all RGB values below 20 are also treated as fully black.
"""

from PIL import Image
import numpy as np
import struct
import sys

def png_to_rgb565_bmp(input_path, output_path, width=160, height=80,
                      alpha_threshold=20, dark_threshold=20):
    # Load image as RGBA and resize
    img = Image.open(input_path).convert("RGBA").resize((width, height))
    arr = np.array(img, dtype=np.uint8)

    # Split channels
    r = arr[:, :, 0].copy()
    g = arr[:, :, 1].copy()
    b = arr[:, :, 2].copy()
    a = arr[:, :, 3]

    # Create mask for pixels that should remain colored
    mask_alpha = a >= alpha_threshold
    mask_rgb = (r >= dark_threshold) | (g >= dark_threshold) | (b >= dark_threshold)
    mask = mask_alpha & mask_rgb

    # Set all other pixels to black
    r[~mask] = 0
    g[~mask] = 0
    b[~mask] = 0

    # Convert to RGB565
    r565 = (r >> 3).astype(np.uint16)
    g565 = (g >> 2).astype(np.uint16)
    b565 = (b >> 3).astype(np.uint16)
    rgb565 = (r565 << 11) | (g565 << 5) | b565

    # BMP headers
    pixel_offset = 54 + 12  # 54-byte BMP header + 12 bytes for color masks
    file_size = pixel_offset + width * height * 2  # 2 bytes per pixel

    with open(output_path, "wb") as f:
        # BMP file header
        f.write(struct.pack("<2sIHHI", b"BM", file_size, 0, 0, pixel_offset))

        # DIB header (BITMAPINFOHEADER)
        f.write(struct.pack("<IIIHHIIIIII",
            40, width, height, 1, 16, 3,
            width * height * 2,
            2835, 2835, 0, 0
        ))

        # Color masks for RGB565
        f.write(struct.pack("<I", 0xF800))  # red mask
        f.write(struct.pack("<I", 0x07E0))  # green mask
        f.write(struct.pack("<I", 0x001F))  # blue mask

        # Pixel data (bottom-up)
        for y in range(height - 1, -1, -1):
            for x in range(width):
                f.write(struct.pack("<H", rgb565[y, x]))

    print(f"✅ Saved {output_path} ({width}x{height}, RGB565, {file_size} bytes)")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python image_convert.py input.png output.bmp [width height]")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]
    w, h = 160, 80  # default size

    if len(sys.argv) == 5:
        w, h = int(sys.argv[3]), int(sys.argv[4])

    png_to_rgb565_bmp(input_file, output_file, w, h)
