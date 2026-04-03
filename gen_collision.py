from PIL import Image

def generate_collision():
    img = Image.open('assets/map.png').convert('RGB')
    w, h = img.size
    TILE = 64
    cols = w // TILE
    rows = h // TILE
    
    grid = [[False for _ in range(cols)] for _ in range(rows)]
    
    for y in range(rows):
        for x in range(cols):
            region = img.crop((x*TILE, y*TILE, (x+1)*TILE, (y+1)*TILE))
            # The void color is basically pitch black. Let's say if mostly < 30 RGB.
            dark_pixels = sum(1 for px in region.getdata() if px[0] < 30 and px[1] < 30 and px[2] < 30)
            if dark_pixels > (TILE*TILE) * 0.5:
                grid[y][x] = True # It's a wall/void
                
    # Now merge into rects
    rects = []
    visited = [[False for _ in range(cols)] for _ in range(rows)]
    
    for y in range(rows):
        for x in range(cols):
            if grid[y][x] and not visited[y][x]:
                # find width
                w_tiles = 0
                while x + w_tiles < cols and grid[y][x+w_tiles] and not visited[y][x+w_tiles]:
                    w_tiles += 1
                    
                # find height
                h_tiles = 0
                valid = True
                while y + h_tiles < rows and valid:
                    for i in range(w_tiles):
                        if not grid[y+h_tiles][x+i] or visited[y+h_tiles][x+i]:
                            valid = False
                            break
                    if valid:
                        h_tiles += 1
                        
                # mark visited
                for j in range(h_tiles):
                    for i in range(w_tiles):
                        visited[y+j][x+i] = True
                        
                rects.append((x*TILE, y*TILE, w_tiles*TILE, h_tiles*TILE))

    # Print C++ code
    with open('map_rects.txt', 'w') as f:
        f.write("obstacles.clear();\n")
        for r in rects:
            f.write(f"obstacles.push_back(sf::FloatRect({r[0]}, {r[1]}, {r[2]}, {r[3]}));\n")
            
    print(f"Generated {len(rects)} rectangles.")

if __name__ == '__main__':
    generate_collision()
