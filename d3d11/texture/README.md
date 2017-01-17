
#Window Creation

![Preview Image](https://github.com/tocchan/guildhall_samples/blob/master/images/d3d11_texture.png "Example Image")

##Setup
__Requires Windows 10 SDK if you want to create a Debug Context!!!__

Builds off of [d3d11/shader](https://github.com/tocchan/guildhall_samples/tree/master/d3d11/shader) to add texturing to our toolbox of shader capability.

This also covers creating a sampler, as well as creating an index buffer to make making the Quad easier.

As I do not want to include actual image loading in these examples for the sake of brevity - I programmatically create a Grid Texture for this example.  

If you want to load imges, I highly recommend using the very well written and concise [stb_image](https://github.com/nothings/stb) code.  If you need something a bit more feature rich, libjpeg and libpng are excellent options.  


##Project Property Chagnes;
- "General -> Project Defaults -> CharacterSet" was changed to "Use Multi-Byte Character Set"
- Added a Post Build step to copy HLSL files to the Output Directory.

##Notes
- I have a hardcoded "Destroy this window when Escape is pressed" inside the windows proc.  Remove this after cleanup.

