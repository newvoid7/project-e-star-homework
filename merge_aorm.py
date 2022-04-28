import imp
import cv2
import numpy as np

ao_img = cv2.imread('.\\resource\\campfire\\ao.jpg', cv2.IMREAD_GRAYSCALE)          # channel R
r_img = cv2.imread('.\\resource\\campfire\\roughness.jpg', cv2.IMREAD_GRAYSCALE)    # channel G
m_img = cv2.imread('.\\resource\\campfire\\metallic.jpg', cv2.IMREAD_GRAYSCALE)     # channel B
# ao_img.astype(np.uint8)
# r_img.astype(np.uint8)
# m_img.astype(np.uint8)
out_img = np.asarray([m_img, r_img, ao_img])
out_img = out_img.transpose((1, 2, 0))
cv2.imwrite('.\\resource\\campfire\\aorm.png', out_img)
