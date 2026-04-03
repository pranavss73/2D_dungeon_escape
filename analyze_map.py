import sys
from PIL import Image

def find_walls():
    try:
        img = Image.open('assets/map.png').convert('L')
        w, h = img.size
        print(f"Map size: {w}x{h}")
        
        TILE = 160 # Based on Dungeon_Tileset.png (160, 160) from sizes.txt
        walls = []
        
        for y in range(0, h, TILE):
            for x in range(0, w, TILE):
                region = img.crop((x, y, x+TILE, y+TILE))
                # Check for dark pixels which often denote walls/void in 2D tops down
                dark_pixels = sum(1 for py in range(TILE) for px in range(TILE) if region.getpixel((px, py)) < 50)
                
                # If more than 40% of the tile is dark, consider it a wall
                if dark_pixels > (TILE * TILE) * 0.4:
                    walls.append((x, y))
                    
        print(f"Found {len(walls)} potential wall tiles.")
        # Just print the first 10 for sanity
        print("First 10 walls:", walls[:10])
        
    except Exception as e:
        print(e)

if __name__ == '__main__':
    find_walls()
