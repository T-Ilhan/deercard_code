# THE TANK DRIVE PROPER FIRST
import asyncio
import threading
import pygame
from bleak import BleakClient

DEVICE_ADDR   = "80:B5:4E:F6:B6:8D"
NUS_RX_UUID   = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
DEADZONE      = 0.2
SEND_INTERVAL = 0.08

latest_cmd = ' '

def axes_to_cmd(left_y, right_y):
    L = -left_y
    R = -right_y
    if abs(L) < DEADZONE: L = 0.0
    if abs(R) < DEADZONE: R = 0.0
    if L == 0.0 and R == 0.0: return ' '
    if L > 0 and R > 0:
        if abs(L - R) < 0.3: return 'w'
        elif L > R: return 'e'
        else: return 'q'
    if L < 0 and R < 0: return 's'
    if L > 0 and R <= 0: return 'd'
    if L <= 0 and R > 0: return 'a'
    return ' '

async def ble_loop():
    global latest_cmd
    while True:
        try:
            print("[INFO] BLE connecting...")
            async with BleakClient(DEVICE_ADDR, timeout=20.0) as client:
                print("[OK] Connected!")
                await client.write_gatt_char(NUS_RX_UUID, b' ', response=True)
                print("[OK] Ready to drive!")
                while True:
                    if not client.is_connected:
                        print("[WARN] Connection lost, reconnecting...")
                        break
                    try:
                        await client.write_gatt_char(NUS_RX_UUID, latest_cmd.encode(), response=False)
                    except Exception as e:
                        print(f"[WARN] Write failed: {e}")
                        break
                    await asyncio.sleep(SEND_INTERVAL)
        except Exception as e:
            print(f"[ERR] BLE error: {e}")
            print("[INFO] Retrying in 2s...")
            await asyncio.sleep(2.0)

def ble_thread():
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    loop.run_until_complete(ble_loop())

if __name__ == "__main__":
    pygame.init()
    pygame.joystick.init()
    joy = pygame.joystick.Joystick(0)
    joy.init()
    print(f"[OK] Controller: {joy.get_name()}")

    t = threading.Thread(target=ble_thread, daemon=True)
    t.start()

    last_cmd = None
    while True:
        pygame.event.pump()
        left_y  = joy.get_axis(1)
        right_y = joy.get_axis(3)
        latest_cmd = axes_to_cmd(left_y, right_y)
        if latest_cmd != last_cmd:
            print(f"  > {latest_cmd}")
            last_cmd = latest_cmd







#LEGACY MODE

# import asyncio
# import threading
# import pygame
# from bleak import BleakClient

# DEVICE_ADDR   = "80:B5:4E:F6:B6:8D"
# NUS_RX_UUID   = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
# DEADZONE      = 0.15
# SEND_INTERVAL = 0.05

# # Command format sent over BLE: "Lsss Rsss\n"
# # where sss is a signed float string, e.g. "L0.75 R-0.50\n"
# # This lets your firmware do proper variable speed.
# # Change LEGACY_MODE = True to fall back to single-char commands.
# LEGACY_MODE = False

# latest_cmd = b'L0.00 R0.00\n'

# def apply_deadzone(val, dz=DEADZONE):
#     if abs(val) < dz:
#         return 0.0
#     # Rescale so output starts from 0 at edge of deadzone
#     sign = 1 if val > 0 else -1
#     return sign * (abs(val) - dz) / (1.0 - dz)

# def axes_to_cmd(left_y, right_x):
#     """
#     Arcade drive mixing:
#       forward  = -left_y   (push up = positive)
#       turn     =  right_x  (push right = positive)
      
#       left_motor  = forward - turn   (clamped -1..1)
#       right_motor = forward +kk4 turn   (clamped -1..1)
#     """
#     forward = apply_deadzone(-left_y)
#     turn    = apply_deadzone(right_x)

#     left_motor  = max(-1.0, min(1.0, forward - turn))
#     right_motor = max(-1.0, min(1.0, forward + turn))

#     if LEGACY_MODE:
#         return legacy_cmd(left_motor, right_motor)
    
#     # Compact BLE string: "L+0.75 R-0.50\n" (14 bytes)
#     return f"L{left_motor:+.2f} R{right_motor:+.2f}\n".encode()

# def legacy_cmd(L, R):
#     """Single-char fallback for original firmware."""
#     THRESH = 0.15
#     if abs(L) < THRESH and abs(R) < THRESH:
#         return b' '
#     if L > THRESH and R > THRESH:
#         if abs(L - R) < 0.3: return b'w'
#         elif L > R:           return b'e'
#         else:                 return b'q'
#     if L < -THRESH and R < -THRESH: return b's'
#     if L > THRESH  and R < -THRESH: return b'd'
#     if L < -THRESH and R > THRESH:  return b'a'
#     return b' '

# async def ble_loop():
#     global latest_cmd
#     while True:
#         try:
#             print("[INFO] BLE connecting...")
#             async with BleakClient(DEVICE_ADDR, timeout=20.0) as client:
#                 print("[OK] Connected!")
#                 await client.write_gatt_char(NUS_RX_UUID, b'L+0.00 R+0.00\n', response=True)
#                 print("[OK] Ready.")
#                 while True:
#                     if not client.is_connected:
#                         print("[WARN] Connection lost, reconnecting...")
#                         break
#                     try:
#                         cmd = latest_cmd
#                         await client.write_gatt_char(NUS_RX_UUID, cmd, response=False)
#                     except Exception as write_err:
#                         print(f"[WARN] Write failed: {write_err}")
#                         break  # exits inner loop -> outer loop reconnects
#                     await asyncio.sleep(SEND_INTERVAL)

#         except Exception as e:
#             print(f"[ERR] BLE error: {e}")
#             print("[INFO] Retrying in 2s...")
#             await asyncio.sleep(2.0)

# def ble_thread():
#     asyncio.set_event_loop_policy(asyncio.WindowsSelectorEventLoopPolicy())
#     loop = asyncio.new_event_loop()
#     asyncio.set_event_loop(loop)
#     loop.run_until_complete(ble_loop())

# if __name__ == "__main__":
#     pygame.init()
#     pygame.joystick.init()
#     joy = pygame.joystick.Joystick(0)
#     joy.init()
#     print(f"[OK] Controller: {joy.get_name()}")

#     t = threading.Thread(target=ble_thread, daemon=True)
#     t.start()

#     last_cmd = None
#     clock = pygame.time.Clock()

#     while True:
#         pygame.event.pump()

#         left_y  = joy.get_axis(1)   # Left stick vertical
#         right_x = joy.get_axis(2)   # Right stick horizontal (axis 2 on most pads)

#         latest_cmd = axes_to_cmd(left_y, right_x)

#         if latest_cmd != last_cmd:
#             print(f"  > {latest_cmd.decode().strip()}")
#             last_cmd = latest_cmd

#         clock.tick(1 / SEND_INTERVAL)  # ~20 Hz poll



# #WIFI COMMAND TRIAL
# import pygame
# import socket
# import time

# TANK_IP       = "192.168.4.1"   # ESP32 AP default IP
# TANK_PORT     = 4210
# DEADZONE      = 0.2
# SEND_INTERVAL = 0.05

# sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# def axes_to_cmd(left_y, right_y):
#     L = -left_y
#     R = -right_y
#     if abs(L) < DEADZONE: L = 0.0
#     if abs(R) < DEADZONE: R = 0.0
#     if L == 0.0 and R == 0.0: return ' '
#     if L > 0 and R > 0:
#         if abs(L - R) < 0.3: return 'w'
#         elif L > R: return 'e'
#         else: return 'q'
#     if L < 0 and R < 0: return 's'
#     if L > 0 and R <= 0: return 'd'
#     if L <= 0 and R > 0: return 'a'
#     return ' '

# if __name__ == "__main__":
#     pygame.init()
#     pygame.joystick.init()
#     joy = pygame.joystick.Joystick(0)
#     joy.init()
#     print(f"[OK] Controller: {joy.get_name()}")
#     print(f"[OK] Sending to {TANK_IP}:{TANK_PORT}")

#     last_cmd = None
#     last_send = 0

#     while True:
#         pygame.event.pump()
#         left_y  = joy.get_axis(1)
#         right_y = joy.get_axis(3)
#         cmd = axes_to_cmd(left_y, right_y)

#         now = time.monotonic()
#         if now - last_send >= SEND_INTERVAL:
#             sock.sendto(cmd.encode(), (TANK_IP, TANK_PORT))
#             last_send = now

#         if cmd != last_cmd:
#             print(f"  > {cmd}")
#             last_cmd = cmd