import time

app = Flask(__name__)
socketio = SocketIO(app)

# Store BPM data
data = []

@app.route('/data', methods=['POST'])
def receive_data():
    global data
    bpm_data = request.json.get('bpm')
    timestamp = time.strftime('%H:%M:%S')
    new_entry = {'time': timestamp, 'bpm': bpm_data}
    data.append(new_entry)
    # Emit new data to connected clients
    socketio.emit('new_data', new_entry)
    return jsonify({'message': 'Data received successfully'}), 200

@app.route('/')
def dashboard():
    return render_template('dashboard.html', data=data)

if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=5000)