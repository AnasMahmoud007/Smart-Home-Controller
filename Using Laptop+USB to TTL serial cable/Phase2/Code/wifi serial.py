import serial
import time
import socket

# ==========================================
# 1. HARDWARE SETUP
# ==========================================
try:
    # Change 'COM5' if Windows assigned a different port to your CP2102
    sensor_port = serial.Serial('COM5', 9600) 
    print("Hardware Sensor connected successfully on COM5")
except Exception as e:
    print(f"Error opening COM port: {e}")
    exit()

# ==========================================
# 2. WI-FI SERVER SETUP
# ==========================================
HOST = '0.0.0.0' # Listen on all available network interfaces
PORT = 8080      # The network port the app will use

server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# This setting prevents the "Port already in use" error if you restart the script quickly
server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) 
server_socket.bind((HOST, PORT))
server_socket.listen(1)

print(f"\nWi-Fi Server Started!")
print(f"-> Open your Android app and connect to your Laptop's IPv4 Address on Port {PORT}")

# ==========================================
# 3. STATE TRACKING VARIABLES
# ==========================================
# Initialize the previous states (Active-Low: True = unpressed, False = pressed)
prev_cts = sensor_port.cts
prev_dsr = sensor_port.dsr
prev_dcd = sensor_port.cd   # Note: PySerial refers to DCD as 'cd'
prev_ri  = sensor_port.ri

# ==========================================
# 4. MAIN SERVER LOOP
# ==========================================
try:
    while True: # OUTER LOOP: Waits for a new connection
        print("\nWaiting for Android App to connect...")
        conn, addr = server_socket.accept()
        print(f"SUCCESS! Connected to Android App at: {addr}")
        
        # Make the connection "non-blocking" so we can instantly detect if the app closes
        conn.setblocking(False) 

        try:
            while True: # INNER LOOP: Handles the active connection
                
                # --- DISCONNECT DETECTION ---
                try:
                    # Try to read 1 byte. If the app closed gracefully, it returns empty bytes (b'')
                    data = conn.recv(1)
                    if data == b'':
                        print("\n[!] Android App disconnected gracefully.")
                        break # Break inner loop, go back to waiting
                except BlockingIOError:
                    pass # Normal behavior: no incoming data, connection is still alive
                except (ConnectionResetError, BrokenPipeError):
                    print("\n[!] Connection lost (Wi-Fi dropped or app forced closed).")
                    break # Break inner loop, go back to waiting
                
                # --- HARDWARE POLLING ---
                curr_cts = sensor_port.cts
                curr_dsr = sensor_port.dsr
                curr_dcd = sensor_port.cd
                curr_ri  = sensor_port.ri

                # --- EDGE DETECTION & TRANSMISSION ---
                # Check Button 1 (CTS)
                if prev_cts == True and curr_cts == False:
                    print("Button 1 (CTS) Pressed!")
                    conn.sendall(b"Node 1: Switch 1 (CTS) Toggled!\r\n")
                    
                # Check Button 2 (DSR)
                if prev_dsr == True and curr_dsr == False:
                    print("Button 2 (DSR) Pressed!")
                    conn.sendall(b"Node 1: Switch 2 (DSR) Toggled!\r\n")

                # Check Button 3 (DCD)
                if prev_dcd == True and curr_dcd == False:
                    print("Button 3 (DCD) Pressed!")
                    conn.sendall(b"Node 1: Switch 3 (DCD) Toggled!\r\n")

                # Check Button 4 (RI)
                if prev_ri == True and curr_ri == False:
                    print("Button 4 (RI) Pressed!")
                    conn.sendall(b"Node 1: Switch 4 (RI) Toggled!\r\n")

                # --- UPDATE STATES ---
                prev_cts = curr_cts
                prev_dsr = curr_dsr
                prev_dcd = curr_dcd
                prev_ri  = curr_ri

                # Debounce delay (50ms)
                time.sleep(0.05) 

        except Exception as e:
            print(f"An unexpected connection error occurred: {e}")
        finally:
            # Safely close the broken connection before waiting for a new one
            conn.close() 

except KeyboardInterrupt:
    print("\nShutting down Smart Home Server...")
    sensor_port.close()
    server_socket.close()