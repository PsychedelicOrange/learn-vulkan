# Learn vulkan repo
## About
In this repo, I have documented my journey in understanding vulkan. My current background I know opengl ( everything on learnopengl.com ) and have made some engines in it.
## Getting started
- First, I watched through this 
[Vulkan lecture series](https://www.youtube.com/watch?v=tLwbj9qys18&list=PLmIqTlJ6KsE1Jx5HV4sd2jOe3V1KMHHgn)  
to get a basic understanding of this api.
- Currently I'm following the instructions in [vulkan-tutorial](https://vulkan-tutorial.com/Development_environment) to setup the repo in c.
- I downloaded the unofficial build of shaderc/glslc instead of glslangvalidator which lookedd more complicated.
- From here on, I made a CmakeLists.txt and ran the test program to enumerate extensions 
- Added some libs from [here](https://github.com/PsychedelicOrange/clibs) which i can develop side by side and use here.
- I tried to add a address sanitizer, which i couldn't figure.
- I tried to `sudo apt install vulkan-sdk` but it was dependent on the `libyaml-cpp0.7` package which was not in the repos `linux-mint`
- I downloaded the vulkan-sdk tarball and installed the includes,bins and libs respective to thier /usr directories
- I followed some https://www.youtube.com/watch?v=0OqJtPnkfC8 till 16mins, for basic advice.
- Now I am gonna proceed with vulkan-tutorial's boiler-plate setup.
