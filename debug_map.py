import sys
from PIL import Image, ImageDraw

def debug_walls():
    img = Image.open('assets/map.png').convert('RGBA')
    overlay = Image.new('RGBA', img.size, (0,0,0,0))
    draw = ImageDraw.Draw(overlay)
    
    w, h = img.size
    TILE = 160
    
    walls = []
    
    # Analyze and draw grid
    for y in range(0, h, TILE):
        for x in range(0, w, TILE):
            region = img.crop((x, y, x+TILE, y+TILE))
            # Convert to LA to easily check darkness
            region_l = region.convert('L')
            dark_pixels = sum(1 for py in range(TILE) for px in range(TILE) if region_l.getpixel((px, py)) < 50)
            
            # Draw grid lines for debugging
            draw.rectangle([x, y, x+TILE, y+TILE], outline=(255,255,255,50))
            
            if dark_pixels > (TILE * TILE) * 0.4:
                walls.append((x, y, TILE, TILE))
                draw.rectangle([x, y, x+TILE, y+TILE], fill=(255,0,0,128))
                
    img = Image.alpha_composite(img, overlay)
    img.save('map_debug_grid.png')
    
    print(f"Generated map_debug_grid.png with {len(walls)} walls.")

if __name__ == '__main__':
    debug_walls()
