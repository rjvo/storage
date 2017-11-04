# About
ROjal_MQTT is MQTT 3.1.1 compliant client. Is support only QoS0 quality level.
It is intened to work in different type of IoT devices. Test codes and examples
uses TCP socket, but socket can be replased by any other session mechanism.

The stack is outcome of JAMK IoT and Cyber security BCs thesis http://www.theseus.fi/handle/TBD...

# License
Copyright 2017 Rami Ojala / JAMK (K5643)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in the
Software without restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

https://opensource.org/licenses/MIT

# API
See include/mqtt.h file for details.

# How to build
## Environment and tools
Linux with cmake and gcc.

## Compiling
### Set required environment variables
* export MQTT_SERVER=123.456.789.000
* export MQTT_PORT=1883

Above variables are used with test codes only (ctest). Defining valid broker (e.g. mosquitto),
makes it possible to run unit and systemtests.

### Create build directory and compile
* mkdir build
* cd build
* cmake ..
* make -j 4

or in single line:
* mkdir build; cd build; cmake ..; make -j 4

### Test functionality
* Run ctest in build directory
* Use rcv tool in build/bin/ directory

# FreeRTOS example
* See FreeRTOS_example/ROjal_MQTT_README.txt for more details

# Source code
Source code is located in src directory and related header files in include directory.
Include directory has mqtt_adaptation.h file, which has a few external functions which
must be changed to be suitable for target environment (Linux and FreeRTOS exists).
