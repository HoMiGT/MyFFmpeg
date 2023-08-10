import MyFFmpeg
import cv2
import numpy as np

# video_path ="http://live3.wopuwulian.com/mp/8yh944a870m542q42aJL44T42rfiDQ.flv?auth_key=1691595770-0-0-4ca211e9d9befcd6ea6ae4acf24a5e4b"
video_path ="../0430.flv"

obj = MyFFmpeg.MyFFmpeg(video_path,0.5,15,30)

status = obj.initialize()
print(status)
if status != 3100:
    exit(-1)

arr = np.zeros(2,dtype=np.int32)
status = obj.video_info(arr)
print(status)
print(arr)


frames = np.zeros((2,arr[1],arr[0],3),dtype=np.uint8)


frame_count = obj.video_frames(frames,2)
print(frame_count)
print(frames[0])
print("=================================")
print(frames[1])
for i in range(frame_count):
    cv2.imwrite(f"{i}.png",frames[i])


# while True:
#     status = obj.decode()
#     print("first:",status)
# # if status != 3100:
# #     exit(-1)


# # frames = obj.frames()
# # print(len(frames))
# # index = 0
# # for frame in frames:
# #     cv2.imwrite(f"{index}.png",frame)
# #     print(index)
# #     index+=1
    

#     status = obj.decode()
#     print("second:", status)
#     if status == 3121 or status == 3125:
#         break
# # if status != 3100:
# #     exit(-1)
# # index = 30
# # for frame in frames:
# #     cv2.imwrite(f"{index}.png",frame)
# #     print(index)
# #     index+=1

# obj.close()

# del obj 

