from PIL import Image
from collections import Counter

def analyze():
    img = Image.open('assets/map.png').convert('RGB')
    w, h = img.size
    colors = Counter(img.getdata())
    print("Most common colors:")
    for color, count in colors.most_common(10):
        print(f"Color: {color}, Count: {count}")

if __name__ == '__main__':
    analyze()
