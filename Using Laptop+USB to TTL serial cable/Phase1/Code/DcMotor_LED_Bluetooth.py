import serial
import serial.tools.list_ports
import time

# --- CONFIGURATION ---
# DTR will control the LED
# RTS will control the MOTOR
# Note: Most CP2102 modules use inverted logic:
# False = High Voltage (ON), True = Low Voltage (OFF)

def list_available_ports():
    print("\n" + "="*50)
    print(" SCANNING FOR HARDWARE PORTS...")
    print("="*50)
    ports = serial.tools.list_ports.comports()
    for i, port in enumerate(ports):
        print(f"[{i+1}] {port.device} - {port.description}")
    return ports

def run_dual_controller():
    ports_list = list_available_ports()
    if not ports_list:
        print("No COM ports detected. Please connect your USB-to-TTL module.")
        return

    # Choose the port (Example: 1 for HC-06, 2 for CP2102)
    print("\n[STEP 1] Setup Bluetooth Connection")
    bt_port = input("Enter COM number for Bluetooth (HC-06): ")
    
    print("\n[STEP 2] Setup Hardware Control")
    hw_port = input("Enter COM number for CP2102 (LED/Motor): ")

    try:
        # Open Serial Connections
        bt_serial = serial.Serial(f"COM{bt_port}", 9600, timeout=0.1)
        hw_serial = serial.Serial(f"COM{hw_port}", 9600, timeout=0.1)

        # Initial States (Start with everything OFF)
        # Setting DTR/RTS to True usually pulls voltage to 0V (OFF)
        hw_serial.dtr = True
        hw_serial.rts = True
        
        led_state = False
        motor_state = False

        print("\n" + "*"*50)
        print("  SYSTEM ONLINE: DUAL CONTROLLER ACTIVATED")
        print("*"*50)
        print(" >> Press 'A' on your phone to toggle the LED")
        print(" >> Press 'B' on your phone to toggle the MOTOR")
        print(" >> Press '0' to Master Reset everything")
        print("*"*50)

        while True:
            if bt_serial.in_waiting > 0:
                raw_data = bt_serial.read().decode('utf-8', errors='ignore').strip().upper()
                
                if not raw_data:
                    continue

                if raw_data == 'A':
                    led_state = not led_state
                    # Toggle DTR (Inverted Logic)
                    hw_serial.dtr = not led_state 
                    status = "LIT UP" if led_state else "DARK"
                    print(f"[DEVICE: LED] Status changed to: {status}")
                    bt_serial.write(f"LED is now {status}\r\n".encode())

                elif raw_data == 'B':
                    motor_state = not motor_state
                    # Toggle RTS (Inverted Logic)
                    hw_serial.rts = not motor_state 
                    status = "SPINNING" if motor_state else "STOPPED"
                    print(f"[DEVICE: MOTOR] Status changed to: {status}")
                    bt_serial.write(f"MOTOR is now {status}\r\n".encode())

                elif raw_data == '0':
                    hw_serial.dtr = True
                    hw_serial.rts = True
                    led_state = False
                    motor_state = False
                    print("[SYSTEM] Master Reset: All Devices OFF")
                    bt_serial.write(b"SYSTEM RESET: ALL OFF\r\n")

    except KeyboardInterrupt:
        print("\nStopping controller...")
    except Exception as e:
        print(f"\n[ERROR] An error occurred: {e}")
    finally:
        if 'bt_serial' in locals(): bt_serial.close()
        if 'hw_serial' in locals(): hw_serial.close()
        print("Ports closed. Goodbye!")

if __name__ == "__main__":
    run_dual_controller()