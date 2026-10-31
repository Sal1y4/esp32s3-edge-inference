# ESP32-S3 Edge Inference: Sensor-Window MLP on ESP-DL

Undergraduate thesis project. An ESP32-S3 board keeps a 12-step history of environmental sensor readings, runs a small INT8-quantized MLP on-device with ESP-DL, and uses the output score to trigger a cleaning routine on a small two-motor robot.

## Pipeline

```
Offline (PC, models/)
  main.py               generate simulated sensor CSV → MinMaxScaler → 12-step windows → Keras MLP → ONNX (tf2onnx)
  quantize_onnx_model.py  ESP-PPQ INT8 post-training quantization (target=esp32s3, 32 calibration windows) → .espdl
  pack_model.py         packs .espdl for the `model` flash partition (invoked by main/CMakeLists.txt at build time)

On-device (components/BSP/MODEL/model.cpp)
  sensors → sliding window (memmove) → normalize 4 features + 2 time features → 1×12×6 float
         → ESP-DL model->run (input quantized to INT8 inside the graph) → score in [0,1]
         → FreeRTOS control task: auto mode triggers cleaning when score > 50%
```

| Item | Value |
| --- | --- |
| Input | 1 × 12 × 6 (temperature, humidity, PM2.5, gas reading, sin/cos time encoding) |
| Network | Dense 72→16→8→1, ReLU / ReLU / Sigmoid, 1,313 parameters |
| Quantization | ESP-PPQ PTQ, INT8 weights/activations, power-of-two scales, INT32 bias, Sigmoid via 256-entry LUT |
| Model file | `models/esp32_model_quantized.espdl`, ~6.5 KiB |
| Toolchain | ESP-IDF 5.4.1, ESP-DL 3.1.1, Python 3.9 (see `models/conda_esp_model_python3_9_21_requirements.txt`) |

## Layout

```
main/                 app_main, FreeRTOS tasks (sensor+inference, control, touch)
components/BSP/MODEL  model load / preprocess / run / postprocess
components/BSP/*      drivers: DHT11, DC01 (PM2.5, UART), MQ + light (ADC), DRV8833 motors, BLE GATT server, touch, LED
models/               training, quantization, packing scripts and model assets
partitions-16MiB.csv  includes a 4 MiB `model` data partition
```

## Build

```
idf.py set-target esp32s3
idf.py build flash monitor
```
The build packs `CONFIG_MODEL_FILE_PATH` (default `models/esp32_model_quantized.espdl`) and flashes it to the `model` partition. ESP-DL is pulled by the component manager (`main/idf_component.yml`). This repository snapshot has not been rebuilt since the original hardware runs.

## Provenance

- Model training, quantization, packing and the on-device inference path (`models/`, `components/BSP/MODEL/`, sensor/control tasks) were written for this project.
- The BLE GATT server is adapted from the ESP-IDF `gatt_server_service_table` example; some sensor drivers are adapted from vendor example code. Parts of the code were written with AI assistance.
- The companion WeChat mini-program used for manual control is not part of this repository.

## Known limitations

These are documented deliberately; they are the first things to fix in a follow-up.

- **Simulated training data.** `main.py` generates the training CSV (sinusoid + noise) and labels it with a threshold rule (6-point mean PM2.5 > 90 or gas > 110). The model has not been evaluated on real household data.
- **Train/deploy preprocessing mismatch.** Training min-max scales all six features; firmware scales only the four sensor features and passes raw sin/cos. Training rows are hourly; on device a new row arrives roughly every 2.5 s. Temperature/humidity are integers on device but continuous in training.
- **Input range.** Values outside the training range saturate at the INT8 input (exponent −7 → representable range [−1, 0.992]). DHT11 readings are clamped in the driver; PM2.5 and gas are not.
- **No isolated quantization-error measurement.** `test_esp32_model.py` holds a historical PC-vs-board comparison, but the two sides used different time encodings, so the gap is not attributable to quantization alone.
- **Control loop.** The auto-clean routine is a blocking `vTaskDelay` sequence (~2.3 s), so a stop command is not read until it finishes; motors are on/off only (no PWM). Shared `g_sensor` is accessed by several tasks without synchronization.
