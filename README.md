# Chat
A simple client/server chat application written in C.  

## Installation guide
The project rely on ncurses librairy so if you don't have it installed on your system, do so by typing:  
`$ sudo apt-get install libncurses-dev`

Then simply compile the sources:  
```
$ git clone http://github.com/jhoukem/Chat
$ cd Chat
$ make
```

Once done you will have to start a server on a terminal:  
`$ ./server [server_port]`  
Now open another terminal and connect a client to that server:  
`$ ./client [server_address] [server_port]`

(The server password is currently defined in the source code. I know it is bad and I should move it to a conf file.)  

## Demo
<img src="https://cloud.githubusercontent.com/assets/9862039/26371402/9b32ff3c-3fc8-11e7-9391-65075f73f1d9.png" alt="Chat demo" />

## Known issues
- The text inside the input area will disappear if the window resizing is too small (even though the text will still be in the buffer and thus will be printed when pressing "Enter")
- There is a bug with the get_input function, sometimes the output is messed up due to a poor code for that function (I don't have the time to refactor it feel free to PR).

