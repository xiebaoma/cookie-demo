# HTTP Cookie Demo - C++ Project

This project demonstrates the basic principles of the HTTP protocol, specifically focusing on **cookies**. It includes both a **client** and a **server** that communicate via HTTP, with the server supporting both **Windows** and **Linux** operating systems.

## Project Overview

This project simulates the process of setting, sending, and receiving cookies through HTTP requests and responses. The server will send cookies in the response headers, and the client will include these cookies in subsequent requests, illustrating how cookies persist between requests across sessions.

## Features

- **Client**:
  - Sends HTTP requests to the server.
  - Handles cookies by storing and sending them with subsequent requests.
  
- **Server**:
  - Responds to client requests with HTTP responses.
  - Sets cookies in the response headers.
  - Supports both Windows and Linux systems.

## Technologies Used

- **C++** for both client and server implementations.
- **Sockets** for communication between client and server.
- **HTTP Protocol** for structuring the requests and responses.

## Setup and Installation

### Prerequisites

- **C++ compiler** (GCC for Linux, MSVC for Windows)
- **CMake** for building the project.
- **Linux** or **Windows** environment (both are supported).

### Installation Steps

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/xiebaoma/cookie-demo.git
   cd cookie-demo
   ```

2. **Build the Project**:
   - For **Linux**:
     ```bash
     mkdir build
     cd build
     cmake ..
     make
     ```
   - For **Windows**:
     ```bash
     mkdir build
     cd build
     cmake ..
     cmake --build .
     ```

3. **Run the Server**:
   - On **Linux**:
     ```bash
     ./server
     ```
   - On **Windows**:
     ```bash
     server.exe
     ```

4. **Run the Client**:
   - On **Linux**:
     ```bash
     ./client
     ```
   - On **Windows**:
     ```bash
     client.exe
     ```

## How It Works

1. **Client Request**:
   The client sends a simple HTTP GET request to the server. If the server has previously set a cookie, the client will include the cookie in the request headers.

2. **Server Response**:
   The server processes the request and responds with an HTTP response that includes a `Set-Cookie` header if it needs to set a new cookie or update an existing one.

3. **Cookie Storage**:
   The client stores the cookies received in the response headers, and includes them in the headers of future requests.

4. **Cookie Persistence**:
   The server can read cookies from the client's request, allowing for persistent sessions or tracking of previous interactions.

## Example Flow

1. **First Request** (client sends request, server sets cookie):
   ```
   GET / HTTP/1.1
   Host: localhost:8080
   ```

   **Server Response** (sets cookie):
   ```
   HTTP/1.1 200 OK
   Set-Cookie: session_id=abc123; Max-Age=3600; Path=/;
   ```

2. **Subsequent Request** (client sends cookie back):
   ```
   GET / HTTP/1.1
   Host: localhost:8080
   Cookie: session_id=abc123
   ```

   **Server Response** (handles the cookie):
   ```
   HTTP/1.1 200 OK
   ```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.