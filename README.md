# KYC/KYT Analysis System for Ethereum Network (C++ / Python)

This project is a system for analyzing the Ethereum network for KYC (Know Your Customer) and KYT (Know Your Transaction) purposes.

## Asynchronous Data-Miner (Python)

This part collects data from the blockchain. It uses the `web3.py` framework and the Alchemy API provider.

The algorithm traverses the transaction graph within set limits (by width and depth). To work in real time (streaming), the collected data is continuously sent to the computing core through an asynchronous message queue.

## High-Performance Analytical Core (C++)

This is the server part. It receives data streams over the network, builds a dependency graph, and generates final reports.

### Optimization (Data-Oriented Design)

To achieve maximum performance, the graph uses the Structure of Arrays (SoA) pattern. Vertex data is split into independent arrays by characteristics.

This provides spatial locality of data. When filtering or searching by a specific attribute, data is read linearly. This efficiently fills the CPU cache lines (L1/L2 cache) and minimizes cache misses.


### Preparation

1. **Get an API key:**
   - Sign up at [alchemy.com](https://www.alchemy.com/)
   - Create a new app and copy the HTTPS endpoint URL

2. **Configure environment variables:**
   Create a `.env` file in the project root with:
   ```env
   ALCHEMY_HTTP_URL=https://eth-mainnet.g.alchemy.com/v2/YOUR_API_KEY

   
## How to Test the System

To test how this system works, follow these steps:

1. Create a folder for building the C++ server:

   ```bash
   mkdir build
   ```

2. Create the build file:

   ```bash
   cmake ..
   ```

3. Build the server application:

   ```bash
   make
   ```

4. Run the server:

   ```bash
   ./server
   ```

5. Open a new terminal window.

6. Open the project folder.

7. Run the asynchronous parser:

   ```bash
   poetry run start
   ```

The parser will build a dependency graph with the width and depth you set. It starts from the crypto wallet address you provide. Then it sends the graph to the server over the local network. The server processes the graph and sends back a response, which is printed to the console.