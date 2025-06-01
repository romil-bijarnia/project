import time
import requests
import RPi.GPIO as GPIO

# — CONFIGURATION —
FIREBASE_URL   = "https://plant-monitor-fb8c1-default-rtdb.firebaseio.com"
POLL_INTERVAL  = 5           # seconds between database polls
SERVO_PIN      = 18          # GPIO18 (pin 12) for servo signal

# — SETUP RPi.GPIO & PWM —
GPIO.setmode(GPIO.BCM)
GPIO.setup(SERVO_PIN, GPIO.OUT)
pwm = GPIO.PWM(SERVO_PIN, 50)  # 50 Hz → 20 ms period
pwm.start(0)                   # start with 0% duty

def set_servo_angle(angle):
    """
    angle: 0–180 degrees.
    Duty cycle ≈ 2% (0°) to 12% (180°).
    """
    duty = (angle / 18.0) + 2.0
    pwm.ChangeDutyCycle(duty)
    time.sleep(0.5)
    pwm.ChangeDutyCycle(0)

def fetch_motor_status():
    """
    GET /commands/motor.json from Firebase.
    Returns True if JSON is true, False otherwise.
    """
    try:
        url = f"{FIREBASE_URL}/commands/motor.json"
        r = requests.get(url, timeout=5)
        r.raise_for_status()
        return r.json() is True
    except Exception as e:
        print(f"[!] Error fetching motor status: {e}")
        return False

def main_loop():
    print("Starting servo‐control loop. Polling every", POLL_INTERVAL, "seconds.")
    last_state = None

    try:
        while True:
            motor_on = fetch_motor_status()
            if motor_on != last_state:
                if motor_on:
                    print("→ Firebase says ON → moving servo to 90°")
                    set_servo_angle(90)
                else:
                    print("→ Firebase says OFF → moving servo to 0°")
                    set_servo_angle(0)
                last_state = motor_on

            time.sleep(POLL_INTERVAL)
    except KeyboardInterrupt:
        print("\nStopping (KeyboardInterrupt). Cleaning up GPIO.")
    finally:
        pwm.stop()
        GPIO.cleanup()

if __name__ == "__main__":
    main_loop()

