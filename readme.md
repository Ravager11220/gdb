

---

# Network Buffer Debugging: Investigating Lifetime & Ownership Bugs in C++

This repository documents a step-by-step root cause analysis of a runtime crash (`SIGSEGV`) in a simulated C++ network packet processing buffer.

The goal of this exercise was to go beyond static code analysis, using **GDB (GNU Debugger)** to observe memory states, stack frame transitions, and heap object lifetimes in real time.

---

## The Symptom

The application initialized a `NetworkBuffer` object containing dynamically allocated packet payloads, passed it to `process_network_data()`, and attempted to verify the packets afterward in `main()`.

While `process_network_data()` executed without throwing errors, subsequent reads to `net_buf.get_packets()[0].payload` inside `main()` triggered a memory access violation (`SIGSEGV`).

```text
Initializing Network Interface...
[+] Processing 3 network packets...
  -> Packet ID: 1000 | Data: PAYLOAD_DATA_00
  -> Packet ID: 1001 | Data: PAYLOAD_DATA_01
  -> Packet ID: 1002 | Data: PAYLOAD_DATA_02

Program received signal SIGSEGV, Segmentation fault.

```

---

## GDB Investigation & Isolation Process

To isolate the point of failure without making assumptions, I ran the executable under GDB TUI (`gdb ./packet_parser`) and tracked memory state across function calls.

1. **Step-Through Payload Monitoring:**
I placed a breakpoint at `process_network_data()` and monitored the payload pointer of the 0th index packet (`packets[0].payload`) at each step. Throughout the entire loop execution inside `process_network_data()`, the payload memory remained valid and intact.
2. **Stack Frame Exit Observation:**
Upon stepping out of `process_network_data()` back into `main()`, I re-inspected `net_buf.get_packets()[0].payload`. Immediately following the function exit, the memory held by the payload field was inaccessible.
3. **Identifying Implicit Cleanup:**
Because there were no explicit calls to `delete` inside `process_network_data()`, I set a breakpoint on `NetworkBuffer::~NetworkBuffer()`. GDB confirmed that the destructor fired automatically right as `process_network_data()` returned.

---

## Root Cause Analysis

Tracing the execution flow exposed a classic **Shallow Copy / Double Free** vulnerability resulting from implicit C++ pass-by-value semantics:

```text
[main: net_buf] ──────► packets: 0x55555556d6c0 ──┐
                                                 ├───► [ Shared Heap Data ]
[process: buffer] ────► packets: 0x55555556d6c0 ──┘        └── payload: 0x7FFFF000

```

1. **Pass-by-Value Copying:**
The function signature `void process_network_data(NetworkBuffer buffer)` accepted the object by value. Since `NetworkBuffer` lacked a custom copy constructor, C++ executed a default bitwise copy.
2. **Shared Pointer Addresses:**
Primitive types like `count` were copied correctly, but the raw pointers (`packets` and `payload`) were copied as raw memory addresses. Both `net_buf` in `main()` and the temporary `buffer` inside `process_network_data()` pointed to the exact same heap addresses.
3. **Premature Destruction:**
When `process_network_data()` reached the end of its scope, its local `buffer` instance was destroyed. The implicit `~NetworkBuffer()` call executed `delete[]` on the shared heap memory addresses.
4. **Dangling Pointer Access:**
Upon returning to `main()`, `net_buf` was left holding dangling pointers to already-freed heap memory, leading directly to the `SIGSEGV` when dereferencing `payload`.

---

## Key Takeaways & Remediation

* **Pass-by-Reference:** Functions accepting non-primitive objects should pass by reference (`const NetworkBuffer& buffer`) to prevent unwanted object copies and premature destructor invocations.
* **The Rule of Three / Five:** Any class managing raw heap allocation (`new`/`delete`) must explicitly define or delete its copy constructor, copy assignment operator, and destructor.
* **Modern C++ RAII:** Replacing raw pointers with automatic memory management containers (`std::vector<Packet>` or `std::unique_ptr`) eliminates manual memory management bugs at compile time.

---

## How to Build & Run

```bash
# Compile with debug symbols and no optimization
g++ -g -O0 packet_parser.cpp -o packet_parser

# Run inside GDB
gdb ./packet_parser

```
