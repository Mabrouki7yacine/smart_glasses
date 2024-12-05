import threading, cv2, datetime, re, unidecode, os
from flask import Flask, jsonify, request
import pytesseract
from googletrans import Translator
import face_recognition
import ast

app = Flask(__name__)

info = {
    "data": ""
}

menu = 0
menu_changed = 1

url = 'http://192.168.43.1:8080/video'
#url = 'http://192.168.100.4:8080/video'
#url = 'http://172.16.43.44:8080/video'

# Open and read the file
with open("my_faces.txt", "r") as f:
    content = f.read()
try:
    my_faces = ast.literal_eval(content)
except (SyntaxError, ValueError) as e:
    print(f"Error parsing the file: {e}")

def rescaleFrame(frame, scale=0.5):
    width = int(frame.shape[1] * scale)
    height = int(frame.shape[0] * scale)
    dimensions = (width, height)
    return cv2.resize(frame, dimensions, interpolation=cv2.INTER_AREA)

def clean_text(text):
    # Remove unwanted characters and replace newlines with spaces
    text = re.sub(r'[|]', '', text)  # Remove specific unwanted characters
    text = text.replace('\n', ' ')  # Replace newlines with spaces
    text = unidecode.unidecode(text)
    return text

@app.route('/upload', methods=['POST'])
def upload_frame():
    file_data = request.data
    #np_img = np.frombuffer(file_data, np.uint8)
    #image_data = cv2.imdecode(np_img, cv2.IMREAD_COLOR)
    file_path = os.path.join('uploads', 'frame.jpg')
    # Write the image data to a file
    with open(file_path, 'wb') as f:
        f.write(file_data)
        print('ok')
    return 'Updated successfully!', 200
    #np_img = np.frombuffer(file_data, np.uint8)
    #image_data = cv2.imdecode(np_img, cv2.IMREAD_COLOR)
@app.route('/upload', methods=['GET'])
def donkey():
    return "posti ya rassa !!!!", 200

@app.route('/', methods=['GET'])
def naaah():
    return "ekhroj", 200

@app.route('/home', methods=['GET'])
def get_home():
    global menu
    menu = 0
    formatted_time = datetime.datetime.now().strftime("%H:%M")
    info = {"data": formatted_time}
    response = jsonify(info), 200
    return response

@app.route('/face_rec', methods=['GET'])
def get_face_rec():
    global menu, menu_changed
    if menu != 1 :
        info['data'] = "wait processing" 
        menu_changed = 1
    else :
        menu_changed = 0
    menu = 1
    response = jsonify(info), 200
    return response

@app.route('/translation', methods=['GET'])
def get_translation():
    global menu, menu_changed
    if menu != 2 :
        info['data'] = "wait processing" 
        menu_changed = 1
    else :
        menu_changed = 0
    menu = 2
    response = jsonify(info), 200
    return response

@app.route('/AI_assistant', methods=['GET'])
def get_AI_assistant():
    global menu
    menu = 3
    info = {"data": "  this option is not available yet"}
    response = jsonify(info), 200
    return response

def handle():
    global info, my_faces
    while True: 
        while menu == 1:
            # face recognition
            cap = cv2.VideoCapture(url)
            ret, frame = cap.read()
            frame = rescaleFrame(frame , 0.5)   
            frame = cv2.rotate(frame, cv2.ROTATE_90_CLOCKWISE) 
            if menu_changed == 1:
                info['data'] = "wait processing"
            try:            
                unknown_face_encodings = face_recognition.face_encodings(frame)
                if not unknown_face_encodings:
                    print("No face found in the unknown image.")
                    info['data'] = "No face found"
                    continue
                for unknown_face_encoding in unknown_face_encodings:
                    for i in range(len(my_faces)) :
                        result = face_recognition.compare_faces([my_faces[i]['encodings']], unknown_face_encoding)
                        if result[0]:
                            info['data'] = my_faces[i]['name']  
                            break 
                if not result[0]:
                    info['data'] = 'unknown'  
                cap.release
            except Exception as e:
                print(f"Error in face recognition: {e}")
                #info['data'] = f"Error in face recognition: {e}"
                info['data'] = "Error"

        while menu == 2 :
            # text translation
            cap = cv2.VideoCapture(url)
            ret, frame = cap.read()
            frame = rescaleFrame(frame , 0.5)   
            frame = cv2.rotate(frame, cv2.ROTATE_90_CLOCKWISE) 
            if menu_changed == 1 :
                info['data'] = "wait processing"
            text = pytesseract.image_to_string(frame)
            #print(text)
            if text is not  None :
                try : 
                    translator = Translator()
                    # Translate the text to French
                    translated = translator.translate(text, src='en', dest='fr')
                    cleaned_text = clean_text(translated.text)
                    #cleaned_text = clean_text(text)
                    info['data'] = cleaned_text
                    print(cleaned_text)
                except Exception as e:
                    print(f"Error in text rec: {e}")
                    #info['data'] = f"Error in face recognition: {e}"
                    info['data'] = "Error"                    
            else : 
                cleaned_text = "no text detected"
                info['data'] = cleaned_text
                print(cleaned_text)

if __name__ == '__main__':
    show_thread = threading.Thread(target=handle)
    show_thread.daemon = True
    show_thread.start()
    app.run(host='0.0.0.0', port=5000, threaded=True)  # Run Flask in threaded mode
