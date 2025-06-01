# motor_control.py

import time
import requests
import RPi.GPIO as GPIO

# 1. — CONFIGURATION SECTION —
# Replace with your actual Firebase URL (no trailing slash).
FIREBASE_URL = "https://plant-monitor-fb8c1-default-rtdb.firebaseio.com"

# How often (in seconds) to check the database.
POLL_INTERVAL = 5

# GPIO pin where the motor‐driver input is connected.
MOTOR_GPIO_PIN = 17  # GPIO17 (pin 11 on the 40-pin header)

# 2. — SET UP RPi.GPIO —
GPIO.setmode(GPIO.BCM)
GPIO.setup(MOTOR_GPIO_PIN, GPIO.OUT)
GPIO.output(MOTOR_GPIO_PIN, GPIO.LOW)  # Ensure motor is off at start

def fetch_motor_status():
    """
    Perform an HTTP GET to Firebase to read the boolean under /commands/motor.json.
    Returns True if the value is the literal JSON true, False otherwise.
    """
    try:
        url = f"{FIREBASE_URL}/commands/motor.json"
        response = requests.get(url, timeout=5)
        response.raise_for_status()
        # The database node should return either true or false (no quotes).
        return response.json() is True
    except Exception as e:
        print(f"[!] Error fetching motor status: {e}")
        return False

def set_motor(on):
    """
    Given a boolean 'on', drive the GPIO pin HIGH (to turn motor on)
    or LOW (to turn motor off).
    """
    if on:
        GPIO.output(MOTOR_GPIO_PIN, GPIO.HIGH)
        print("→ Motor ON")
    else:
        GPIO.output(MOTOR_GPIO_PIN, GPIO.LOW)
        print("→ Motor OFF")

def main_loop():
    print("Starting motor‐control loop. Polling every", POLL_INTERVAL, "seconds.")
    try:
        while True:
            motor_on = fetch_motor_status()
            set_motor(motor_on)
            time.sleep(POLL_INTERVAL)
    except KeyboardInterrupt:
        print("\nStopping (KeyboardInterrupt). Cleaning up GPIO.")
    finally:
        GPIO.output(MOTOR_GPIO_PIN, GPIO.LOW)
        GPIO.cleanup()

if __name__ == "__main__":
    main_loop()

