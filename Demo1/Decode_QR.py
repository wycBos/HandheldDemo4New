import cv2
def decode(): 
    image = cv2.imread('QR_UserID.png')
    qrCodeDetector = cv2.QRCodeDetector()
    data, bbox, straight_qrcode = qrCodeDetector.detectAndDecode(image)
    if bbox is not None:
        return data 
    else:
        return -1