# import asyncio
# from bleak import BleakClient, BleakScanner
# from pynput import keyboard

# NUS_RX = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
# DEVICE_NAME = "DeerPlatform"
# SEND_INTERVAL = 0.05  # 50ms -- well within the 150ms ESP32 timeout

# held = set()

# def get_cmd():
#     if 'w' in held: return b'w'
#     if 's' in held: return b's'
#     if 'a' in held: return b'a'
#     if 'd' in held: return b'd'
#     return None

# def on_press(key):
#     try:
#         k = key.char.lower() if hasattr(key, 'char') and key.char else None
#     except Exception:
#         k = None
#     if k in ('w', 'a', 's', 'd'):
#         held.add(k)
#     if key == keyboard.Key.space:
#         held.add(' ')

# def on_release(key):
#     try:
#         k = key.char.lower() if hasattr(key, 'char') and key.char else None
#     except Exception:
#         k = None
#     held.discard(k)
#     if key == keyboard.Key.space:
#         held.discard(' ')
#     if key == keyboard.Key.esc:
#         return False

# async def main():
#     print(f"Scanning for '{DEVICE_NAME}'...")
#     device = await BleakScanner.find_device_by_name(DEVICE_NAME, timeout=10.0)
#     if device is None:
#         print("Device not found. Make sure the ESP32 is powered on and not already connected.")
#         return

#     print(f"Found {device.name} ({device.address}), connecting...")

#     async with BleakClient(device) as client:
#         print("Connected! Hold WASD to drive, SPACE to stop, ESC to quit.\n")

#         listener = keyboard.Listener(on_press=on_press, on_release=on_release)
#         listener.start()

#         try:
#             while listener.is_alive():
#                 cmd = get_cmd()
#                 if cmd:
#                     await client.write_gatt_char(NUS_RX, cmd, response=False)
#                 await asyncio.sleep(SEND_INTERVAL)
#         except Exception as e:
#             print(f"Error: {e}")
#         finally:
#             try:
#                 await client.write_gatt_char(NUS_RX, b' ', response=False)
#             except:
#                 pass
#             listener.stop()
#             print("Disconnected.")

# asyncio.run(main())

import asyncio
from bleak import BleakClient, BleakScanner
from pynput import keyboard

NUS_RX = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
DEVICE_NAME = "DeerPlatform"
SEND_INTERVAL = 0.05  # 50ms -- well within the 150ms ESP32 timeout

held = set()

def get_cmd():
    if 'q' in held and 'w' not in held: return b'q'
    if 'e' in held and 'w' not in held: return b'e'
    if 'w' in held: return b'w'
    if 's' in held: return b's'
    if 'a' in held: return b'a'
    if 'd' in held: return b'd'
    return None

def on_press(key):
    try:
        k = key.char.lower() if hasattr(key, 'char') and key.char else None
    except Exception:
        k = None
    if k in ('w', 'a', 's', 'd', 'q', 'e'):
        held.add(k)
    if key == keyboard.Key.space:
        held.add(' ')

def on_release(key):
    try:
        k = key.char.lower() if hasattr(key, 'char') and key.char else None
    except Exception:
        k = None
    held.discard(k)
    if key == keyboard.Key.space:
        held.discard(' ')
    if key == keyboard.Key.esc:
        return False

async def main():
    print(f"Scanning for '{DEVICE_NAME}'...")
    device = await BleakScanner.find_device_by_name(DEVICE_NAME, timeout=10.0)
    if device is None:
        print("Device not found. Make sure the ESP32 is powered on and not already connected.")
        return

    print(f"Found {device.name} ({device.address}), connecting...")

    async with BleakClient(device) as client:
        print("Connected! Hold WASD to drive, SPACE to stop, ESC to quit.\n")

        listener = keyboard.Listener(on_press=on_press, on_release=on_release)
        listener.start()

        try:
            while listener.is_alive():
                cmd = get_cmd()
                if cmd:
                    await client.write_gatt_char(NUS_RX, cmd, response=False)
                await asyncio.sleep(SEND_INTERVAL)
        except Exception as e:
            print(f"Error: {e}")
        finally:
            try:
                await client.write_gatt_char(NUS_RX, b' ', response=False)
            except:
                pass
            listener.stop()
            print("Disconnected.")

asyncio.run(main())