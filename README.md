# 🏫 Smart Classroom Monitoring System

**Final Project - CS1 (Computer Science 1)**

A comprehensive sensor monitoring system designed for smart classroom environments in electrical engineering education. This system implements real-time data collection, anomaly detection, and data persistence with a client-server architecture.

---

## 👥 Team Members
- **Ali Reza** - 2306211471
- **Mikola Syabila** - 2306266741  
- **Fadlin Alwan Hanafi** - 2306224335
- **Muhammad Alif Iqbal** - 2306206654

---

## 🎯 Project Overview

This system monitors classroom environmental conditions using three key sensors:
- **🌡️ Temperature**: Optimal range 18-26°C for learning environments
- **💧 Humidity**: Maintained between 30-70% for comfort
- **💡 Light Intensity**: 300-1000 lux for adequate visibility

### Key Features
- ✅ **Real-time Monitoring**: Live sensor data collection and processing
- ✅ **Anomaly Detection**: Automatic identification of environmental issues
- ✅ **Data Persistence**: Binary and JSON export capabilities
- ✅ **Client-Server Architecture**: Distributed monitoring system
- ✅ **Interactive CLI**: User-friendly command-line interface
- ✅ **Comprehensive Testing**: 40+ unit and integration tests

---

## 🏗️ System Architecture

```
┌─────────────────┐    Network     ┌─────────────────┐
│     Client      │ ──────────────→ │     Server      │
│  (Data Source)  │                 │ (Data Processor)│
└─────────────────┘                 └─────────────────┘
                                            │
                                            ▼
                    ┌─────────────────────────────────────┐
                    │          Data Manager               │
                    │    (Anomaly Detection & Query)     │
                    └─────────────────────────────────────┘
                                            │
                                            ▼
                    ┌─────────────────────────────────────┐
                    │         Data Storage                │
                    │    (Binary Files & JSON Export)    │
                    └─────────────────────────────────────┘
```

### Core Components

1. **Client (`Client.cpp/hpp`)**
   - Simulates sensor data collection
   - Establishes TCP connections to server
   - Automatic reconnection on connection loss
   - Configurable data transmission intervals

2. **Server (`Server.cpp/hpp`)**
   - Handles multiple client connections
   - Real-time data processing and validation
   - Integration with DataManager for anomaly detection
   - Callback system for data processing notifications

3. **DataManager (`DataManager.cpp/hpp`)**
   - Anomaly detection with configurable thresholds
   - Advanced querying and filtering capabilities
   - Statistical analysis and data sorting
   - Memory-efficient data management

4. **DataStorage (`DataStorage.cpp/hpp`)**
   - Binary file persistence for performance
   - JSON export for data analysis and reports
   - Batch operations for large datasets
   - Error handling for file I/O operations

5. **AnomalyDetector (`AnomalyDetector.cpp/hpp`)**
   - Multi-parameter anomaly detection
   - Configurable threshold system
   - Detailed anomaly classification
   - Batch anomaly analysis

---

## 🚀 Getting Started

### Prerequisites
- **Windows 10/11** (Primary development environment)
- **Visual Studio 2019/2022** with C++ support
- **CMake 3.15+**
- **Google Test Framework** (included in project)

### Building the Project

1. **Clone and Navigate**:
```bash
git clone <repository-url>
cd finpro
```

2. **Build with CMake**:
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

3. **Verify Installation**:
```bash
# Run all tests
.\tests\run_tests.exe

# Should output: [  PASSED  ] 40 tests.
```

### Running the Application

The system supports three execution modes:

#### 1. 🖥️ Interactive CLI Mode (Default)
```bash
.\finpro.exe
```
**Features**:
- Interactive menu system with emoji indicators
- Real-time data generation and analysis
- Manual anomaly detection
- Query system with filtering and sorting
- Data export capabilities

#### 2. 🌐 Server Mode
```bash
.\finpro.exe server 8080
```
**Features**:
- Listens for client connections on specified port
- Real-time data processing with anomaly detection
- Automatic data persistence
- Multi-client support

#### 3. 📡 Client Mode
```bash
.\finpro.exe client 127.0.0.1 8080
```
**Features**:
- Connects to specified server
- Continuous sensor data transmission
- Automatic reconnection on network issues
- Configurable transmission intervals

### 🎮 Quick Demo

Use the provided demo script for a complete system demonstration:
```bash
.\demo.bat
```

This script will:
1. Start the server in background
2. Run a client simulation
3. Demonstrate all system features
4. Show real-time anomaly detection

---

## 📊 Data Format

### Sensor Data Structure
```cpp
struct SensorData {
    uint64_t timestamp;     // Unix timestamp in milliseconds
    double temperature;     // Temperature in Celsius
    double humidity;        // Humidity percentage (0-100)
    double lightIntensity;  // Light intensity in lux
    bool isAnomalous;       // Anomaly detection flag
    double deviation;       // Statistical deviation score
};
```

### Network Protocol
```
Format: "Timestamp (ms): 1640995200000, Temp: 22.50 C, Humidity: 45.30 %, Light: 500.00 lux"
Response: "OK" or "ERROR"
```

### File Formats

**Binary Storage**: Efficient binary format for high-performance storage and retrieval
**JSON Export**: Human-readable format for analysis and reporting
```json
{
  "anomalies": [
    {
      "timestamp": 1640995200000,
      "temperature": 35.0,
      "humidity": 80.0,
      "lightIntensity": 50.0,
      "deviation": 2.5
    }
  ]
}
```

---

## 🔧 Configuration

### Anomaly Detection Thresholds
```cpp
// Default thresholds (can be customized)
Temperature: 18.0°C - 26.0°C
Humidity: 30.0% - 70.0%
Light Intensity: 300.0 - 1000.0 lux
```

### Network Settings
```cpp
// Default server settings
Port: 8080
Max Connections: 10
Connection Timeout: 5 seconds
Retry Attempts: 3
```

---

## 🧪 Testing

### Test Coverage
Our comprehensive test suite includes **40 tests** covering:

- **Unit Tests**: Individual component functionality
- **Integration Tests**: Component interaction and data flow
- **Network Tests**: Client-server communication
- **Storage Tests**: Data persistence and retrieval
- **Anomaly Detection Tests**: Detection accuracy and edge cases

### Running Tests
```bash
# Run all tests
.\tests\run_tests.exe

# Sample output:
[==========] Running 40 tests from 5 test suites.
[  PASSED  ] 40 tests.
```

### Test Categories
1. **DataManagerTest** (8 tests) - Query operations and data management
2. **ClientTest** (6 tests) - Network communication and reconnection
3. **ServerTest** (4 tests) - Multi-client handling and integration
4. **DataStorageTest** (11 tests) - File I/O and data persistence
5. **AnomalyDetectorTest** (11 tests) - Anomaly detection algorithms

---

## 📁 Project Structure

```
finpro/
├── 📂 src/                          # Source code
│   ├── Client.cpp                   # Client implementation
│   ├── Server.cpp                   # Server implementation
│   ├── DataManager.cpp              # Data management and queries
│   ├── DataStorage.cpp              # File I/O operations
│   ├── AnomalyDetector.cpp          # Anomaly detection logic
│   └── main.cpp                     # Multi-mode application entry
├── 📂 include/                      # Header files
│   ├── Client.hpp                   # Client class definitions
│   ├── Server.hpp                   # Server class definitions
│   ├── DataManager.hpp              # Data management interfaces
│   ├── DataStorage.hpp              # Storage interfaces
│   ├── AnomalyDetector.hpp          # Detection interfaces
│   └── SensorData.hpp               # Data structures
├── 📂 tests/                        # Test suite
│   ├── test_client.cpp              # Client functionality tests
│   ├── test_server.cpp              # Server and integration tests
│   ├── test_data_manager.cpp        # Data management tests
│   ├── test_data_storage.cpp        # Storage functionality tests
│   └── test_anomaly_detector.cpp    # Anomaly detection tests
├── 📂 build/                        # Build artifacts
│   ├── finpro.exe                   # Main executable
│   └── tests/run_tests.exe          # Test runner
├── 📄 CMakeLists.txt                # Build configuration
├── 📄 demo.bat                      # Demo script
└── 📄 README.md                     # This file
```

---

## 🎯 Usage Examples

### Example 1: Interactive Data Analysis
```bash
.\finpro.exe
# Select option 1: Generate and analyze sensor data
# Select option 3: Query data with filters
# Select option 4: Export anomalies to JSON
```

### Example 2: Distributed Monitoring
```bash
# Terminal 1 - Start server
.\finpro.exe server 8080

# Terminal 2 - Start client
.\finpro.exe client 127.0.0.1 8080
```

### Example 3: Custom Anomaly Detection
```cpp
// In your code
AnomalyDetector detector;
detector.setTemperatureRange(20.0, 24.0);  // Stricter temperature range
detector.setHumidityRange(40.0, 60.0);     // Narrower humidity range
```

---

## 🔍 Troubleshooting

### Common Issues

**Build Errors**:
- Ensure Visual Studio C++ components are installed
- Verify CMake version is 3.15 or higher
- Check that all dependencies are properly linked

**Network Connection Issues**:
- Verify firewall settings allow the application
- Check if the specified port is available
- Ensure server is running before starting client

**Test Failures**:
- Run tests in Release mode for optimal performance
- Check for antivirus interference
- Verify all test data files are accessible

### Performance Optimization
- Use Release build configuration for production
- Consider adjusting data transmission intervals for network efficiency
- Monitor memory usage for large datasets

