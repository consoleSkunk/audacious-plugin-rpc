# audacious-plugin-rpc
A Discord Rich Presence plugin for the Audacious music player!

**NOTE:** This project is still using the old Discord RPC framework. Please use a [modern fork]([https://github.com/](https://github.com/onegentig/audacious-discord-rpc) to unlock new functionality, including the listening progress bar.

# Usage
1. Download the current release from the [releases page](https://github.com/darktohka/audacious-plugin-rpc/releases).
2. Extract `libaudacious-plugin-rpc.so` into the folder `/usr/lib/audacious/General/`.
3. Open Audacious, go to Settings > Plugins and enable the `Discord RPC` plugin.

# Screenshots
![Screenshot 1](https://i.imgur.com/fmSBkpt.png)
![Screenshot 2](https://i.imgur.com/INHK64d.png)

# Compilation
1. Clone the repository.
2. Compile and install the plugin:
```
mkdir build
cd build
cmake ..
make install
```
