import time, array
# from machine import Pin, ADC

# cv_in = ADC(Pin(26))
# outs = [Pin(27, Pin.OUT),
#         Pin(28, Pin.OUT),
#         Pin(29, Pin.OUT),
#         Pin(0, Pin.OUT),
#         Pin(3, Pin.OUT),
#         Pin(4, Pin.OUT),
#         Pin(2, Pin.OUT),
#         Pin(1, Pin.OUT)]

speed = 0.1
position = 0.0
last_time = time.perf_counter()
direction = 1 # 1 = up, -1 = down
light_levels = array.array('f', [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0])

while True:
  delta = time.perf_counter() - last_time
  last_time = time.perf_counter()
  distance = speed * delta

  position += (distance * direction)
  position = min(1, max(0, position))
  
  if position == 1:
    direction = -1

  if position == 0:
    direction = 1

  for num in range (8):
    light_levels[num] = 1 - abs(position - num / 7)

  print(position)
  print(light_levels)
