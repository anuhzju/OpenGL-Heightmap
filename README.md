CSCI-420 Computer Graphics
Assignment #1 - Heightmap Visualization
Anna Zhu

This program takes in black and white jpeg images and visualizes it as a heightmap. It has four modes, activated by these corresponding keys:

"1" - Vertex Mode: displays each pixel as a point
"2" - Wireframe Mode: displays a "grid" of horizontal and vertical lines connecting the pixels
"3" - Triangle Mode (flat): displays a surface produced by triangles
"4" - Triangle Mode (Smooth): displays a smoothed surface produced through averaging

The program allows for the following controls:
- rotation with mouse control (default)
- scale with mouse control while holding "shift"
- translation with mouse control while holding "control" 

When in smooth shading mode ("4"):
- increase height scale with "="
- decrease height scale with "-"
- increase height exponent with "9" 
- decrease height exponent with "0"
- loop through additional colormaps with "[" and "]"

