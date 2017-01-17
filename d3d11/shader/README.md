#Window Creation

![Preview Image](https://github.com/tocchan/guildhall_samples/blob/master/images/d3d11_shader.png "Example Image")

##Setup
Builds off of [d3d11/setup](https://github.com/tocchan/guildhall_samples/tree/master/d3d11/setup), and adds an examples for how a Shader is compiled from HLSL to ByteCode, and then farther compiled into a runnable Shader Program.  

To demonstrate it working, this demo also includes a quick example on how to create a Vertex Buffer and an Input Layout.


##Project Property Changes;
- "General -> Project Defaults -> CharacterSet" was changed to "Use Multi-Byte Character Set"
- Added a Post Build step to copy HLSL files to the Output Directory.  


##Notes
- I have a hardcoded "Destroy this window when Escape is pressed" inside the windows proc.  Remove this after cleanup.

