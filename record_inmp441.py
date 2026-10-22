import time
import wave
import serial

PORT = "COM5"
RATE = 16000
SECONDS = 4
BYTES_TO_READ = RATE * SECONDS * 2  # 16-bit mono samples

filename = input("WAV filename to save: ").strip()

with serial.Serial(PORT, 921600, timeout=2) as board:
    time.sleep(2)  # allow the ESP32 to restart when the port opens
    board.reset_input_buffer()

    input("Get ready. Press Enter, then say the phrase once: ")
    print("Recording now—speak clearly.")
    board.write(b"R")

    audio = bytearray()
    while len(audio) < BYTES_TO_READ:
        chunk = board.read(BYTES_TO_READ - len(audio))
        if not chunk:
            raise TimeoutError("Recording stopped early. Check the COM port and wiring.")
        audio.extend(chunk)

with wave.open(filename, "wb") as wav:
    wav.setnchannels(1)
    wav.setsampwidth(2)
    wav.setframerate(RATE)
    wav.writeframes(audio)

print(f"Saved {filename}")