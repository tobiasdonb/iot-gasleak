import urllib.request, json, base64
from PIL import Image

Image.new('RGB', (100, 100)).save('test.jpg')
b64 = base64.b64encode(open('test.jpg', 'rb').read()).decode('utf-8')
req = urllib.request.Request(
    'https://detect.roboflow.com/fire-ynqxb/1?api_key=YOUR_ROBOFLOW_API_KEY&confidence=0.5&format=json',
    data=b64.encode('utf-8'),
    headers={'Content-Type': 'application/x-www-form-urlencoded'}
)

try:
    print('Testing base64:')
    print(urllib.request.urlopen(req).read().decode())
except urllib.error.HTTPError as e:
    print('ERROR:', e.read().decode())
