# MemViz

MemViz is a visualization tool designed to help users understand memory usage and allocation in their applications. This tool provides a graphical interface to visualize memory allocation and deallocation in real time.

## How to Run the Code

To run the MemViz application on your local server, follow these steps:

1. Clone the repository:
   ```bash
   git clone https://github.com/DhruvSPatel6113/MemViz.git
   cd MemViz
   ```

2. Start a local server and open the HTML file:
   ```bash
   # Using Python 3
   python3 -m http.server 8000
   
   # Or using Node.js
   npx http-server
   ```

3. Open your browser and navigate to `http://localhost:8000` to view the MemViz application.

## Modifying the Code

If you want to modify the C++ engine code, you will need to recompile it using Emscripten. Follow these steps:

1. **Install Emscripten** (if not already installed):
   - Follow the [Emscripten installation guide](https://emscripten.org/docs/getting_started/downloads.html)

2. **Navigate to the C++ source directory**:
   ```bash
   cd path/to/cpp/source
   ```

3. **Recompile the engine using Emscripten**:
   ```bash
   emcc your_engine.cpp -o ../path/to/output.js -s WASM=1
   ```

4. **Start the local server** and refresh your browser to see the changes:
   ```bash
   python3 -m http.server 8000
   ```

## Tech Stack

- **C++ (74.2%)**: Core memory visualization engine
- **JavaScript (17.9%)**: Web interface and interaction logic
- **CSS (5.2%)**: Styling and layout
- **HTML (2.7%)**: Structure and markup

## Project Structure

- `/src/cpp/`: C++ engine source code
- `/src/js/`: JavaScript application logic
- `/src/css/`: Stylesheet files
- `/index.html`: Main HTML entry point
- `Makefile` or build script: For Emscripten compilation

## Requirements

- Emscripten (for development/modification)
- A modern web browser
- A local HTTP server

## Contributing

Feel free to fork this repository and contribute to MemViz. For major changes, please open an issue first to discuss what you would like to change.

## License

This project is open source and available under the MIT License.
