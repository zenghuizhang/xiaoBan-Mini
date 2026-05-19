#!/usr/bin/env python3
"""截图捕获工具 — 通过 USB Serial 与 xiaoBan 通信

用法:
    # 主动触发截图 (发送 's'):
    python3 tools/capture.py /dev/ttyACM0 /tmp/screenshot.png

    # 被动接收自动截图 (等待设备输出):
    python3 tools/capture.py /dev/ttyACM0 /tmp/screenshot.png --wait

    # 连续监控模式 (接收所有截图):
    python3 tools/capture.py /dev/ttyACM0 out/ --monitor
"""
import serial, time, sys, os, base64, re, argparse

def capture_one(ser, out_path, send_cmd=True, timeout=15):
    """捕获一张截图"""
    if send_cmd:
        ser.reset_input_buffer()
        ser.write(b's')
        ser.flush()
        time.sleep(0.5)

    b64_data = []
    total_bytes = 0
    t0 = time.time()
    started = False

    while time.time() - t0 < timeout:
        line = ser.readline()
        if not line:
            continue
        try:
            txt = line.decode('utf-8', errors='replace').strip()
        except:
            continue

        if 'SCREENSHOT_START' in txt:
            m = re.search(r'SCREENSHOT_START (\d+)', txt)
            if m:
                total_bytes = int(m.group(1))
                print(f"  Screenshot: {total_bytes} bytes")
                started = True
            continue

        if started and 'SCREENSHOT_END' in txt:
            break

        if started:
            m = re.search(r'SS:([A-Za-z0-9+/=]+)', txt)
            if m:
                b64_data.append(m.group(1))

    if not b64_data or not started:
        print("  ✗ No screenshot data received")
        return False

    b64 = ''.join(b64_data)
    data = base64.b64decode(b64)
    print(f"  Decoded: {len(data)} bytes")

    # 320x240 RGB565
    w, h = 320, 240
    expected = w * h * 2
    if len(data) < expected:
        data = data + b'\x00' * (expected - len(data))
    data = data[:expected]

    import numpy as np
    from PIL import Image
    arr = np.frombuffer(data, dtype='<u2').reshape((h, w))
    r = ((arr >> 11) & 0x1F) * 255 // 31
    g = ((arr >> 5) & 0x3F) * 255 // 63
    b = (arr & 0x1F) * 255 // 31
    rgb = np.stack([r, g, b], axis=-1).astype(np.uint8)
    Image.fromarray(rgb).save(out_path)
    print(f"  ✓ Saved: {out_path} ({os.path.getsize(out_path)} bytes)")
    return True

def main():
    parser = argparse.ArgumentParser(description='xiaoBan 截图捕获工具')
    parser.add_argument('port', nargs='?', default='/dev/ttyACM0', help='串口设备')
    parser.add_argument('output', nargs='?', default='/tmp/screenshot.png', help='输出文件/目录')
    parser.add_argument('--wait', action='store_true', help='被动等待 (不发送命令)')
    parser.add_argument('--monitor', action='store_true', help='持续监控模式')
    parser.add_argument('--count', type=int, default=1, help='截图张数 (非 monitor 模式)')
    args = parser.parse_args()

    ser = serial.Serial(args.port, 115200, timeout=1.0)
    print(f"Connected: {args.port}")

    if args.monitor:
        print("Monitor mode: waiting for screenshots...")
        n = 0
        while True:
            out = f"{args.output}/snap_{n:04d}.png" if os.path.isdir(args.output) else args.output
            if capture_one(ser, out, send_cmd=False, timeout=0):
                n += 1
            else:
                time.sleep(1)
    else:
        for i in range(args.count):
            out = f"{args.output}".format(i=i) if args.count > 1 else args.output
            print(f"\nShot {i+1}/{args.count}...")
            capture_one(ser, out, send_cmd=not args.wait, timeout=15)

    ser.close()

if __name__ == '__main__':
    main()
