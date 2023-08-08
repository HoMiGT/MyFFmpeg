import MyFFmpeg
import cv2
import numpy as np

video_path ="http://live3.wopuwulian.com/mp/4f4d0fj8088K6BIN9jPWU9255O9Y2l.flv?auth_key=1691518724-0-0-8d58a7212fdbe53b8bf60713303d9531"

obj = MyFFmpeg.MyFFmpeg(video_path,0.5,15,30,0)

status = obj.initialize()
print(status)
if status != 3100:
    exit(-1)
status = obj.decode()
print(status)
if status != 3100:
    exit(-1)


frames = obj.frames()
print(len(frames))
index = 0
for frame in frames:
    cv2.imwrite(f"{index}.png",frame)
    print(index)
    index+=1
    

status = obj.decode()
print(status)
if status != 3100:
    exit(-1)
index = 30
for frame in frames:
    cv2.imwrite(f"{index}.png",frame)
    print(index)
    index+=1

obj.close()

del obj 

