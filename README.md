# nrsc5

This program receives NRSC-5 digital radio stations using an SDR device like an SDRplay RSP, any SDR supported by SoapySDR, an RTL-SDR dongle, or by reading from I/Q files. It offers a command-line interface as well as an API upon which other applications can be built. Before using it, you'll first need to compile the program using the build instructions below.


## Installing and running the precompiled binaries for SDRplay

### Windows 64 bit - command line NRSC5 decoder

- download and install the latest SDRPlay API version 3.X (currently 3.15) from here: https://www.sdrplay.com/api/
- download and unzip the latest version of the command line NRSC5 decoder for Windows 64 bit from here: https://github.com/fventuri/nrsc5/releases - to extract the program you have to accept the licenses (GPL3 and FAAD2)
- copy the 64 bit SDRplay API dll to the same folder where you have extracted the NRSC5 decoder - the 64 bit SDRplay API dll is normally under `C:\Program Files\SDRplay\API\x64\sdrplay_api.dll`
- run the nrsc5 decoder as follows:
```
nrsc5-64 89.9 0
```
- where the first number is the FM station frequency and second number is the digital channel you want to listen to (more options like antenna and gain selection are fully described below).

### MacOS - command line NRSC5 decoder

- install the prerequisites dependencies (libao, fftw, and portaudio) for MacOS. For instance using the 'brew' package manager, the command would be:
```
brew install libao fftw portaudio
```
- download and install the latest SDRPlay API version 3.X (currently 3.15) from here: https://www.sdrplay.com/api/
- download and unzip the latest version of the command line NRSC5 decoder for MacOS from here: https://github.com/fventuri/nrsc5/releases - to extract the program you have to accept the licenses (GPL3 and FAAD2)
- run the nrsc5 decoder as follows:

```
nrsc5-macos 89.9 0
```
- where the first number is the FM station frequency and second number is the digital channel you want to listen to (more options like antenna and gain selection are fully described below).



## Building on Ubuntu, Debian or Raspbian

Prerequisites and git repository clone:

Note: <SDR_DRIVER> below is one of: rtlsdr, sdrplay, or soapy

    sudo apt install git build-essential cmake autoconf libtool libao-dev libfftw3-dev librtlsdr-dev
    git clone https://github.com/fventuri/nrsc5.git
    cd nrsc5
    mkdir build
    cd build
    cmake -DSDR_DRIVER=<SDR_DRIVER> [options] ..
    make
    sudo make install
    sudo ldconfig

Available build options:

    -DUSE_NEON=ON            Use NEON instructions. [ARM, default=OFF]
    -DUSE_SSE=ON             Use SSSE3 instructions. [x86, default=OFF]
    -DUSE_FAAD2=ON           AAC decoding with FAAD2. [default=ON]
    -DLIBRARY_DEBUG_LEVEL=1  Debug logging level for libnrsc5. [default=5]
    -DBUILD_DOC=ON           Generate html API documentation [default=OFF]
    -DSDR_DRIVER=rtlsdr      Build nrsc5 for RTL-SDR
    -DSDR_DRIVER=sdrplay     Build nrsc5 for SDRplay (SDRplay API version 3)
    -DSDR_DRIVER=soapy       Build nrsc5 for SoapySDR

You can test the program using the included sample capture:

    xz -d < ../support/sample.xz | src/nrsc5 -r - 0

## Building on Fedora

Follow the Ubuntu instructions above, but replace the first command with the following:

    sudo dnf install git make patch cmake autoconf libtool libao-devel fftw-devel rtl-sdr-devel libusb1-devel

## Building on openSUSE

Follow the Ubuntu instructions above, but replace the first command with the following:

    zypper install -t pattern devel_C_C++
    zypper install git cmake libao-devel fftw3-devel rtl-sdr-devel libusb-1_0-devel

## Building on macOS using [Homebrew](https://brew.sh)

    curl https://raw.githubusercontent.com/fventuri/nrsc5/master/nrsc5.rb > /tmp/nrsc5.rb
    brew install --HEAD -s /tmp/nrsc5.rb

## Building for Windows

To build the program for Windows, you can either use [MSYS2](http://www.msys2.org) on Windows, or else use a cross-compiler on an Ubuntu, Debian or macOS machine. Scripts are provided to help with both cases.

### Building on Windows with MSYS2

Install [MSYS2](http://www.msys2.org). Open a terminal using the "MSYS2 MinGW 64-bit" shortcut. (Or use the 32-bit shortcut if you prefer a 32-bit build.)

    pacman -Syu

If this is the first time running pacman, you will be told to close the terminal window. After doing so, reopen using the same shortcut as before.

    pacman -Su
    pacman -S git
    git clone https://github.com/fventuri/nrsc5.git

Note: <SDR_DRIVER> below is one of: rtlsdr, sdrplay, or soapy

    nrsc5/support/msys2-build <SDR_DRIVER>

You can test your installation using the included sample file:

    cd ~/nrsc5/support
    xz -d sample.xz
    nrsc5.exe -r sample 0

If the sample file does not work, make sure you followed all of the instructions. If it still doesn't work, file an issue with the error message. Please put "[Windows]" in the title of the issue.

Once everything is built, you can run nrsc5 independently of MSYS2. Copy the following files from your MSYS2 mingw64 (or mingw32) directory (e.g. C:\\msys64\\mingw64\\bin):

* libnrsc5.dll
* nrsc5.exe

### Cross-compiling for Windows from Ubuntu / Debian

Note: <SDR_DRIVER> below is one of: rtlsdr, sdrplay, or soapy

    sudo apt install cmake autoconf libtool pkgconf git mingw-w64
    git clone https://github.com/fventuri/nrsc5.git
    cd nrsc5
    support/win-cross-compile 64 --cmake-args="-DUSE_SSE=ON -DSDR_DRIVER=<SDR_DRIVER>" -j4

Replace `64` with `32` if you want a 32-bit build. Once the build is complete, copy `*.dll` and `nrsc5.exe` from the `build-win64/bin` (or `build-win32/bin`) folder to your Windows machine.

### Cross-compiling for Windows from macOS

    brew install cmake autoconf automake libtool pkgconf git mingw-w64
    git clone https://github.com/fventuri/nrsc5.git
    cd nrsc5
    support/win-cross-compile 64 --cmake-args="-DUSE_SSE=ON" -j4

Replace `64` with `32` if you want a 32-bit build. Once the build is complete, copy `*.dll` and `nrsc5.exe` from the `build-win64/bin` (or `build-win32/bin`) folder to your Windows machine.

## Usage

### Command-line options:

    frequency                       center frequency in MHz or Hz
                                      (do not provide frequency when reading from file)
    program                         audio program to decode
                                      (0, 1, 2, or 3)
    -g gain                         RTL-SDR: gain
                                      (example: 49.6)
                                      (automatic gain selection if not specified)
                                    SDRplay: LNAstate.IFGR (IFGR=0 -> AGC enabled)
                                    SoapySDR: GainName1=GainValue1,... (AGC=1 to enable AGC)
    -d device-index                 RTL-SDR: rtl-sdr device
                                    SDRplay: RSP serial number
                                    SoapySDR: SoapySDR device arguments (driver, serial, etc)
    -p ppm-error                    ppm error
    -A antenna                      antenna (only SDRplay and SoapySDR)
    -H rtltcp-host                  rtl_tcp host with optional port
                                      (example: localhost:1234)
    -r iq-input                     read IQ samples from input file
    -w iq-output                    write IQ samples to output file
    -o audio-output                 write audio to output file
    -t audio-type                   type of audio output (wav or raw)
                                      (default is wav. used in conjunction with -o)
    -q                              disable log output
    -l log-level                    set log level
                                      (1 = DEBUG, 2 = INFO, 3 = WARN)
    -v                              print the version number and exit
    --am                            receive AM signals
                                      (default is FM)
    -T                              enable bias-T
    -D direct-sampling-mode         enable direct sampling
                                      (1 = I-ADC input, 2 = Q-ADC input)
    --dump-aas-files dir-name       dump AAS files
                                      (WARNING: insecure)
    --dump-hdc file-name            dump HDC packets

### Examples:

Tune to 107.1 MHz and play audio program 0:

    nrsc5 107.1 0

Tune to 107.1 MHz and play audio program 0. Manually set gain to 49.0 dB and save raw IQ samples to a file:

    nrsc5 -g 49.0 -w samples1071 107.1 0

Read raw IQ samples from a file and play back audio program 0:

    nrsc5 -r samples1071 0

Tune to 90.5 MHz and convert audio program 0 to WAV format for playback in an external media player:

    nrsc5 -o - 90.5 0 | mplayer -

### Keyboard commands:

To switch between audio programs at runtime, press <kbd>0</kbd> through <kbd>7</kbd>.

To quit, press <kbd>Q</kbd>.

### RTL-SDR drivers on Windows

If you get errors trying to access your RTL-SDR device, then you may need to use [Zadig](http://zadig.akeo.ie/) to change the USB driver. Once you download and run Zadig, select your RTL-SDR device, ensure the driver is set to WinUSB, and then click "Replace Driver". If your device is not listed, enable "Options" -> "List All Devices".

### Application Programming Interface (API)

If you would like to build an application that makes use of nrsc5's functionality, you can use the [C API](include/nrsc5.h) ([documentation](https://theori-io.github.io/nrsc5/c-api/)) or [Python API](support/nrsc5.py). The [`nrsc5` command-line application](src/main.c) is built on top of the C API, and an equivalent [Python command-line application](support/cli.py) is built on top of the Python API. These applications serve as examples of how to use the API.

Note: When using the Python API or the Python command-line application on Windows, place `libnrsc5.dll` in the same folder as `nrsc5.py`.

### Other notes

- This SDRplay and SoapySDR components of this program have only been tested under Linux
- To use the sdrplay driver with SoapySDR, you need to allow for the specific NRSC5 sampling rate (744187.5 Hz); the version in the master branch of the driver SoapySDRPlay3 does not currently support this sample rate, so you'll need a modified version that you can find here: https://github.com/fventuri/SoapySDRPlay3/tree/nrsc5-sampling-rate (repository: https://github.com/fventuri/SoapySDRPlay3.git - branch: nrsc5-sampling-rate)
