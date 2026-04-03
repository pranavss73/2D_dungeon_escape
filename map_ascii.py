import sys
from PIL import Image

def analyze_map(tile_size):
    img = Image.open('assets/map.png').convert('L')
    w, h = img.size
    cols = w // tile_size
    rows = h // tile_size
    
    ascii_map = []
    
    for y in range(rows):
        row_str = ""
        for x in range(cols):
            region = img.crop((x*tile_size, y*tile_size, (x+1)*tile_size, (y+1)*tile_size))
            dark_count = sum(1 for px in list(region.getdata()) if px < 80)
            
            total = tile_size * tile_size
            if dark_count > total * 0.5:
                row_str += "X" # Solid wall
            elif dark_count > total * 0.25:
                row_str += "x" # Partial wall
            else:
                row_str += "." # Floor
        ascii_map.append(row_str)
        
    with open('real_map_ascii.txt', 'w', encoding='utf-8') as f:
        f.write(f"--- Map ASCII ({tile_size}x{tile_size} tiles) ---\n")
        f.write("    " + "".join([str(i%10) for i in range(cols)]) + "\n")
        for i, r in enumerate(ascii_map):
            f.write(f"{i:02d}: {r}\n")

if __name__ == '__main__':
    analyze_map(64)
