# Distributed Node-Oriented Soft Mesh Protocol Design and Implementation (Soft Mesh)

## 1. Introduction

### 1.1 Background

During research on existing open-source solutions, it was found that WiFi cards supporting the IEEE 802.11s standard can implement Mesh network interfaces at the data link layer and theoretically deploy the B.A.T.M.A.N. solution. However, for WiFi cards that do not support this standard but can operate in both STA and AP modes, Mesh network functionality can still be implemented through software. We call such solutions "Soft Mesh." Currently, there is no universal open-source Soft Mesh version available. Although some embedded development boards (like the ESP32) have Mesh functionality, manufacturers have encapsulated the Mesh functionality in proprietary binaries, limiting user flexibility and making it difficult to meet specific business requirements in certain scenarios.

For WiFi cards that do not support the IEEE 802.11s standard, Mesh networks cannot be directly implemented. However, in hardware supporting both WiFi STA and AP modes simultaneously, the functionality of a Mesh network can be realized through software-defined methods. The early ESP8266 platform implemented a similar functionality with easyMesh, proving the feasibility of Soft Mesh. The Soft Mesh solution aims to provide an easy way to implement Mesh networks for development boards that do not support the IEEE 802.11s standard but support STA and AP modes.

### 1.2 Purpose

This project aims to design and implement a software-based Mesh network solution to address the challenges of Mesh networking with WiFi cards that do not support the IEEE 802.11s standard. The solution utilizes the coexistence of STA and AP modes in WiFi to implement Mesh functionality at the application layer, offering an extensible, portable, and universal Soft Mesh solution that can support various hardware platforms and communication methods such as Bluetooth and StarFlash.

### 1.3 Overview of the Solution

The Soft Mesh solution is based on development boards that support both STA and AP modes. These modes are not limited to WiFi communication but can also be abstracted for other relay communication methods, enabling the development boards to connect to other nodes and be connected by other nodes. To enhance portability, this solution abstracts the hardware interface layer, making it theoretically hardware-agnostic and adaptable to other communication methods like Bluetooth and StarFlash. The solution builds a self-organizing, modular, and high-portability Mesh network with an N-ary tree structure (N representing the maximum number of connections an AP supports), further improving the applicability of Soft Mesh.

### 1.4 Technical Design and Implementation

The Soft Mesh solution implements Mesh functionality at the application layer, requiring only the migration of WiFi driver interfaces and provision of standard socket interfaces. This design not only implements WiFi Mesh network functionality but also takes portability and scalability into account at the code structure level. By decoupling hardware interfaces, the Soft Mesh solution offers excellent portability, making it easy to apply to other development boards with relay functionality or other communication methods.

### 1.5 Expected Goals

1. **Decentralization**: The network can self-heal after any node failure.
2. **Self-Organization**: Nodes autonomously select connections and build the network.
3. **Node Interconnectivity**: Supports multi-hop communication between any two nodes.
4. **High Portability**: The Mesh network code is decoupled from hardware drivers and supports Bluetooth, StarFlash, and other communication methods.
5. **Modular Design**: Independent development of each module allows for easy comparison and optimization of different algorithms.

## 2. Project Structure

### 2.1 Software Architecture

The software architecture is as follows:

```mathematica
+-----------------------------------------------------------+
|                    Application Interface Layer (API)      |
|       - Mesh Initialization Interface    - Data Transmission Interface |
|       - Broadcast and Unicast Interface    - Network Status Check Interface |
+-----------------------------------------------------------+
|             Node Management and Optimization Layer          |
|       - Node Election and Merging     - Node Load Balancing and Optimization |
|       - Low Power Sleep Management   - Fault and Network Optimization |
+-----------------------------------------------------------+
|               Security and Authentication Layer             |
|       - Node Authentication        - Data Encryption and Decryption |
|       - Key Management            - Secure Connection Management |
+-----------------------------------------------------------+
|             Network State Management Layer                  |
|       - Network Initialization       - STA/AP Connection Management |
|       - Root Node Creation           - Root Node Conflict Detection |
|       - Network Scanning            - Neighbor Node Management |
+-----------------------------------------------------------+
|             Routing and Data Transport Layer                |
|       - Routing Data Packet Management    - MAC/IP Binding Management |
|       - Data Packet Sending and Receiving   - Network Topology Monitoring and Synchronization |
+-----------------------------------------------------------+
|               Hardware Abstraction Layer (HAL)             |
|       - Wi-Fi Configuration Interface       - AP and STA Mode Management |
|       - Data Packet Transmission      - Bluetooth/StarFlash Reserved Interface |
+-----------------------------------------------------------+
|               Testing Layer                              |
|       - Wi-Fi Functionality Testing   - Wireless Interface Testing |
|       - Mesh API Testing       - Routing and Transmission Testing |
|       - Network Status Testing    - Routing Layer Message Queue Testing |
+-----------------------------------------------------------+
```

### 2.2 Repository Directory Structure

The directory structure of the repository is as follows:

```bash
/soft_mesh
│
├── CMakeLists.txt                 # Project root build file
├── LICENSE                        # Open source license
├── README.md                      # Chinese README document
├── README.en.md                   # English README document
├── /hal                           # Hardware Abstraction Layer (HAL)
│   ├── CMakeLists.txt             # HAL build file
│   ├── /inc                       # HAL header files
│   │   ├── hal_wifi.h             # Wi-Fi operation interface definition
│   │   └── hal_wireless.h         # Wireless communication interface definition
│   ├── /src                       # HAL implementation files
│   │   ├── CMakeLists.txt         # HAL implementation build file
│   │   ├── hal_wifi.c             # Wi-Fi interface implementation
│   │   └── hal_wireless.c         # Wireless communication implementation
│   └── /test                      # HAL testing files
│       ├── CMakeLists.txt         # Testing build file
│       ├── test_wifi.c            # Wi-Fi functionality test
│       └── test_wireless.c        # Wireless functionality test
│
├── /mesh_api                      # Mesh Network API Layer
│   ├── CMakeLists.txt             # Mesh API module build file
│   ├── /inc                       # Mesh API header files
│   │   └── mesh_api.h             # Core Mesh API interface definitions
│   ├── /src                       # Mesh API implementation files
│   │   ├── CMakeLists.txt         # Mesh implementation build file
│   │   └── mesh_api.c             # Core Mesh API implementation
│   └── /test                      # Mesh API testing files
│       ├── CMakeLists.txt         # Testing build file
│       └── test_api.c             # Mesh API test
│
├── /network                       # Network State Management Layer
│   ├── CMakeLists.txt             # Network state management module build file
│   ├── /inc                       # Network state management header files
│   │   └── network_fsm.h          # Network FSM interface definitions
│   ├── /src                       # Network state management implementation files
│   │   ├── CMakeLists.txt         # Network implementation build file
│   │   └── network_fsm.c          # Network state management logic implementation
│   └── /test                      # Network state management testing files
│       ├── CMakeLists.txt         # Testing build file
│       └── test_network.c         # Network state management test
│
└── /routing_transport             # Routing and Transport Layer
    ├── CMakeLists.txt             # Routing and transport module build file
    ├── /inc                       # Routing and transport header files
    │   └── routing_transport.h    # Routing and transport core API definitions
    ├── /src                       # Routing and transport implementation files
    │   ├── CMakeLists.txt         # Routing implementation build file
    │   └── routing_transport.c    # Data packet routing and transmission implementation
    └── /test                      # Routing and transport testing files
        ├── CMakeLists.txt         # Testing build file
        └── test_routing.c         # Routing and transport test
```

## 3. Installation and Environment Configuration

This project is based on Hi3863E development. Please configure the Hi3863E compilation environment by referring to the [Quick Start Guide](https://gitee.com/HiSpark/fbb_ws63/tree/master/tools). After setting up the environment, place the source code of this repository in the `application\samples` folder. Then, add the following configuration code at the appropriate location in the `application\samples\CMakeLists.txt`.

```cmake
if(DEFINED CONFIG_ENABLE_SOFT_MESH_SAMPLE)
    add_subdirectory_if_exist(soft_mesh)
endif()
```

Add the following code at the appropriate location in `application\samples\Kconfig`:

```makefile
config ENABLE_SOFT_MESH_SAMPLE
    bool
    prompt "Enable the Sample of soft mesh."
    default n
    depends on SAMPLE_ENABLE
    help
        This option means enable the sample of soft mesh.
```

Once the configuration is successful, click the system configuration button, and you should see the configuration code we added. Start **Enable the sample of soft mesh**, and it should show a checkmark, indicating a successful selection. **Note: Make sure to close any other wireless communication programs**. Otherwise, other wireless programs may conflict with some hardware-related functions, causing the Soft Mesh program to fail.

![system_config](README.assets/system_config.png)

After enabling the Soft Mesh configuration, recompile and execute the program to compile.

## 4. Usage Example

`application\samples\soft_mesh\mesh_api\test\test_api.c` is the code to test the API layer interface. Enable the compilation of `test_api` in `application\samples\soft_mesh\mesh_api\test\CMakeLists.txt` (it is enabled by default).

The code of `test_api.c` is as follows. This code creates a thread to execute the `test_api_task` function, which first initializes the Mesh network configuration. It then enters an infinite loop waiting for network joining. Once connected, it non-blockingly receives messages sent to the node from the message queue. If a message is received, it responds with "Hi, Node!".

In this test scenario, one node will broadcast data to other nodes in the Mesh network by uncommenting the broadcast function in the code.

Flash multiple nodes with the non-broadcasting code.

Flash one node with the broadcasting code.

After powering on the nodes, check the results via the serial port.

```c
#include <stdio.h>
#include <string.h>
#include "cmsis_os2.h"
#include "app_init.h"
#include "mesh_api.h"

// Define macro switches to enable or disable log output
#define ENABLE_LOG 1  // 1 means enable logging, 0 means disable logging

// Define LOG macro: if ENABLE_LOG is 1, print logs with filename, line number, and log content
#if ENABLE_LOG
    #define LOG(fmt, ...) printf("LOG [%s:%d]: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
    #define LOG(fmt, ...) do { UNUSED(fmt); } while (0) 
#endif

void test_api_task(void)
{
    LOG("test_api_task run...\n");
    // Set the Mesh network SSID and password
    if(mesh_init("FsrMesh", "12345678") != 0) {
        LOG("mesh_init failed.\n");
    } else {
        LOG("mesh_init successfully.\n");
    }
    while (1)
    {
        if (mesh_network_connected() == 0) {
            LOG("Mesh network not connected.\n");
            osDelay(100);
            continue;
        }
        osDelay(100);
        // Uncomment below line to enable broadcasting
        // if (mesh_broadcast("Hello, Mesh!") != 0) {
        //     LOG("mesh_broadcast failed.\n");
        // } else {
        //     LOG("mesh_broadcast successfully.\n");
        // }
        char src_mac[7] = {0};
        src_mac[6] = '\0';
        char data[512] = {0};
        if(mesh_recv_data(src_mac, data) == 0) {
            LOG("Received data from client: %s, MAC: %s\n", data, src_mac);
        } else {
            LOG("No data received.\n");
            continue;
        }
        LOG("Say hi to mac: %s\n", src_mac);
        mesh_send_data(src_mac, "Hi, Node!");
    }
}

/* Create task */
static void sta_sample_entry(void)
{
    osThreadAttr_t attr;
    attr.name       = "test_api_task";
    attr.attr_bits  = 0U;
    attr.cb_mem     = NULL;
    attr.cb_size    = 0U;
    attr.stack_mem  = NULL;
    attr.stack_size = 0x1000;
    attr.priority   = osPriorityLow4;

    if (osThreadNew((osThreadFunc_t)test_api_task, NULL, &attr) == NULL) {
        LOG("Create test_api_task failed.\n");
    } else {
        LOG("Create test_api_task successfully.\n");
    }
}

/* Start task */
app_run(sta_sample_entry);
```

## 5. API Documentation

This API documentation provides core interfaces for the implementation of the Soft Mesh network, including Wi-Fi management, wireless communication module operations, Mesh network initialization and data transmission, network status management, routing, and data packet transmission. Below is a detailed description of the main APIs. To reduce the coupling of the code, users should only call functions related to mesh functionality in `mesh_api.h`. If the required function is not provided by `mesh_api.h`, users can implement the corresponding function and encapsulate it in `mesh_api.c`.

### 5.1 Mesh API Interface (mesh_api.h)

Provides core operation interfaces for Mesh network, including initialization, connection status detection, and data transmission:

**Network Initialization:**

`mesh_init()`: Initializes the Mesh network, setting the SSID and password.

**Data Transmission:**

`mesh_send_data()`: Sends data to a specified MAC address. `mesh_broadcast()`: Broadcasts data to all nodes. `mesh_recv_data()`: Receives data packets.

**Connection Status:**

`mesh_network_connected()`: Checks the network connection status.

### 5.2 Routing and Transport Management (routing_transport.h)

This module manages routing and data packet transmission in the Mesh network, supporting core multi-node communication functionality:

**Data Packet Management:**

`generate_data_packet()`: Generates a data packet. `broadcast_data_packet()`: Broadcasts a data packet. 

`send_data_packet()`: Sends a data packet to a specified MAC address.

**Routing Management:**

`route_transport_task()`: Executes the routing transport task, maintaining the network topology.

### 5.3 Network Status Management (network_fsm.h)

This module implements the network state machine, processing device scanning, connection, and Mesh network creation and maintenance operations under different states:

**State Machine Management:**

`network_fsm_init()`: Initializes the network state machine. 

`network_fsm_run()`: Enters the state machine main loop to process state transitions.

**State Handling:**

`state_scanning()`: Scans and selects the best Mesh network. `state_create_root()`: Creates a new root node.

### 5.4 Wireless Communication Management (hal_wireless.h)

This module provides hardware abstraction layer interfaces for different types of wireless communication (Wi-Fi, Bluetooth, StarFlash), supporting module initialization, connection, scanning, data transmission, and other operations.

**Initialization and Management:**

`HAL_Wireless_Init()`: Initializes the specified wireless communication module, configuring resources. 

`HAL_Wireless_Deinit()`: Releases the resources of the specified wireless communication module. 

`HAL_Wireless_Start()`: Starts the wireless communication module (e.g., Wi-Fi STA mode, Bluetooth connection mode). 

`HAL_Wireless_Stop()`: Stops the wireless communication module.

**AP Mode and Scanning:**

`HAL_Wireless_EnableAP()`: Enables AP mode (e.g., Wi-Fi AP or Bluetooth broadcasting), configuring SSID, password, channel, etc. 

`HAL_Wireless_DisableAP()`: Disables AP mode. 

`HAL_Wireless_Scan()`: Scans for wireless networks of the specified type and stores the results in `results`.

**Connection and Device Information:**

`HAL_Wireless_Connect()`: Connects to a specified wireless device (e.g., Wi-Fi AP). 

`HAL_Wireless_Disconnect()`: Disconnects from the current wireless connection. 

`HAL_Wireless_GetIP()`: Retrieves the current IP address (Wi-Fi only). 

`HAL_Wireless_GetConnectedDeviceInfo()`: Retrieves connected device information (e.g., MAC address, signal strength). 

`HAL_Wireless_GetAPMacAddress()`: Retrieves the MAC address of the AP mode. 

`HAL_Wireless_GetNodeMAC()`: Retrieves the MAC address of the current node.

**Data Transmission:**

`HAL_Wireless_SendData_to_child()`: Sends data to a specified child (or upper-level) node. 

`HAL_Wireless_SendData_to_parent()`: Sends data to the parent node. 

`HAL_Wireless_ReceiveData()`: Receives data from other nodes. 

`HAL_Wireless_ReceiveDataFromClient()`: Receives data from a client, returning the MAC address and data content.

**Server Management:**

`HAL_Wireless_CreateServer()`: Creates a wireless reception server. 

`HAL_Wireless_CloseServer()`: Closes the specified server instance.

 `HAL_Wireless_GetChildMACs()`: Retrieves the MAC addresses of all child nodes, returning the number of child nodes.

### 5.5 Wi-Fi Management (hal_wifi.h)

This module includes interfaces related to Wi-Fi initialization, connection, configuration, and data transmission, supporting both STA and AP modes:

**Initialization and De-initialization:**

1. `HAL_WiFi_Init()`: Initializes the Wi-Fi module.
2. `HAL_WiFi_Deinit()`: Releases Wi-Fi resources.

**STA Mode:**

3. `HAL_WiFi_Connect()`: Connects to a specified Wi-Fi network.
4. `HAL_WiFi_GetIP()`: Retrieves the IP address.

**AP Mode:**

5. `HAL_WiFi_AP_Enable()`: Enables AP mode.
6. `HAL_WiFi_GetAPConfig()`: Retrieves AP configuration information.

**Data Transmission:**

7. `HAL_WiFi_Send_data()`: Sends data over Wi-Fi.
8. `HAL_WiFi_Receive_data()`: Receives Wi-Fi data.

These APIs cover the core functions of the Soft Mesh network system, from hardware operations to network communication, data transmission, and routing management, providing complete interface support.

## 6. Design Details

## 7. FAQ and Troubleshooting

Answers to common questions and solutions to possible issues encountered during operation.

## 8. Contribution Guidelines

Thank you for your interest and support in this project! We welcome community members to contribute code or suggest improvements. Please follow the steps and guidelines below to ensure a smooth contribution process:

1. Fork the repository
   Fork this project's repository to your personal account on GitHub or Gitee.

2. Clone the repository
   Clone the forked repository to your local development environment:

   ```bash
   git clone <your repository URL>  
   cd <project folder>  
   ```

3. Create a branch
   Please create a new branch for your changes to avoid modifying the master branch directly. The branch name should be concise and describe the feature or bug fix.

   ```bash
   git checkout -b feature/your-feature-name
   ```
   
4. Follow coding conventions
   Ensure that your code follows the project's coding style and conventions to maintain consistency and maintainability. You can use code formatting tools (such as clang-format) to format your code.
   Comments should be clear and concise, so that other developers can quickly understand the code logic.

5. Commit message conventions
   The project follows the Conventional Commits standard to maintain a clear commit history. Please write your commit messages in the following format:

   ```php
   <type>(<scope>): <subject>
   ```
   
   - `type`: Describes the category of the change.
   - `scope`: The area of the change, such as the module name, folder, or specific function.
   - `subject`: A brief description of the change, starting with a verb in lowercase.

**Common commit types**

| Type     | Description                                                  |
| -------- | ------------------------------------------------------------ |
| feat     | New feature                                                  |
| fix      | Bug fix                                                      |
| docs     | Documentation change                                         |
| style    | Code formatting changes (does not affect logic, such as spaces or semicolons) |
| refactor | Code refactoring (no new features or bug fixes)              |
| perf     | Performance improvement                                      |
| test     | Add or modify tests                                          |
| chore    | Changes to the build process or auxiliary tools              |

**Commit examples**

```bash
git commit -m "feat(api): add data transmission for child nodes"  
git commit -m "fix(network): correct connection timeout issue"  
git commit -m "docs(readme): update usage instructions"  
```

1. Submit a Pull Request
   After completing the code changes, follow these steps to submit a Pull Request (PR):

   1. Push the changes: Push the changes from your branch to your remote repository.

   ```bash
   git push origin feature/your-feature-name
   ```
   
   1. Create a PR: Create a Pull Request in the main project's repository and briefly describe the changes and the reason for them.
   2. PR Review: The project maintainers will review your code and may suggest improvements. You can update the PR based on the feedback until it meets the project's requirements.
   
2. Suggest Improvements
   If you have suggestions for improvements or have discovered a bug, feel free to submit an issue using the Issues tab. Please provide as much detail as possible, including steps to reproduce the issue and potential solutions, to help the project maintainers handle your feedback more efficiently.

   Note: Before submitting a PR, please ensure that your branch is synchronized with the main branch and resolve any potential merge conflicts.

3. Copyright
   The code you submit will, by default, be subject to the project's open-source license (MIT) and is considered to belong to this project.

## 9. License

This project is licensed under the MIT License. The full text is as follows:

- MIT License: Allows users to freely use, copy, modify, merge, publish, distribute, sublicense, and sell copies of the software, provided that the original copyright and license notices are retained. The code in this project can be used for both commercial and non-commercial purposes.

Please refer to the LICENSE file in the root directory for the full license text and copyright information.
