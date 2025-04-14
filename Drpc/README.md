# Drpc RPC Framework

## Overview
The Drpc project is a lightweight Remote Procedure Call (RPC) framework designed to facilitate communication between distributed systems. It provides a simple and efficient way to invoke methods on remote servers as if they were local calls.

## Project Structure
The project is organized into the following directories:

- **include/**: Contains header files that declare the classes and interfaces for the RPC framework.
  - `Drpcapplication.h`: Declares the `DrpcApplication` class for framework initialization.
  - `Drpcconfig.h`: Defines the `DrpcConfig` class for managing configuration settings.
  - `Drpcchannel.h`: Declares the `DrpcChannel` class for handling communication channels.
  - `Drpccontroller.h`: Defines the `DrpcController` class for managing RPC calls.

- **src/**: Contains source files that implement the functionality declared in the header files.
  - `Drpcapplication.cpp`: Implements the `DrpcApplication` class methods.
  - `Drpcconfig.cpp`: Implements the `DrpcConfig` class methods.
  - `Drpcchannel.cpp`: Implements the `DrpcChannel` class methods.
  - `Drpccontroller.cpp`: Implements the `DrpcController` class methods.

- **CMakeLists.txt**: Configuration file for CMake, specifying project settings and build instructions.

## Setup Instructions
1. Clone the repository to your local machine.
2. Navigate to the project directory.
3. Create a build directory:
   ```
   mkdir build
   cd build
   ```
4. Run CMake to configure the project:
   ```
   cmake ..
   ```
5. Build the project:
   ```
   make
   ```

## Usage
To use the Drpc framework, include the necessary headers in your application and follow the initialization and RPC invocation procedures outlined in the documentation for each class.

## Contributing
Contributions are welcome! Please submit a pull request or open an issue for any enhancements or bug fixes.

## License
This project is licensed under the MIT License. See the LICENSE file for details.